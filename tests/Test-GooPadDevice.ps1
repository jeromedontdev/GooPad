param(
    [Parameter(Mandatory)][uri]$Device,
    [Parameter(Mandatory)][pscredential]$Credential,
    [uri]$Companion,
    [securestring]$CompanionToken
)

$ErrorActionPreference = 'Stop'
if ($Device.Scheme -ne 'https') { throw 'Use the HTTPS device address.' }
$base = $Device.GetLeftPart([System.UriPartial]::Authority)
$token = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes(
    "$($Credential.UserName):$($Credential.GetNetworkCredential().Password)"))
$headers = @{ Authorization = "Basic $token" }
function Request($Path, $Method = 'GET', $Body = $null, $Headers = $headers, $ContentType = 'application/json') {
    $options = @{
        Uri = "$base$Path"; Method = $Method; Headers = $Headers
        SkipCertificateCheck = $true; SkipHttpErrorCheck = $true; TimeoutSec = 30
    }
    if ($null -ne $Body) { $options.Body = $Body; $options.ContentType = $ContentType }
    if ($Body -is [byte[]]) { $options.TimeoutSec = 180 }
    try { Invoke-WebRequest @options }
    catch [System.OperationCanceledException], [System.Net.Http.HttpRequestException] {
        throw "Request $Method $Path failed: $($_.Exception.Message)"
    }
}
function Expect($Response, $Status) {
    if ([int]$Response.StatusCode -ne $Status) { throw "Expected HTTP $Status, got $($Response.StatusCode)" }
}

$original = Request '/api/config'
Expect $original 200
$unauthorized = Request '/api/config' 'POST' '{}' @{}
Expect $unauthorized 401
if ($unauthorized.Content -ne 'Authentication required') { throw 'Unauthorized handler appended data.' }
if ((Request '/api/config').Content -ne $original.Content) { throw 'Unauthorized write changed configuration.' }
$catalog = Request '/api/icons'
Expect $catalog 200
if (!(($catalog.Content | ConvertFrom-Json).icons | Where-Object name -eq 'server')) {
    throw 'Font Awesome catalog is invalid or missing fa-server.'
}

$test = $original.Content | ConvertFrom-Json -AsHashtable
$controls = (Request '/api/status').Content | ConvertFrom-Json
$soundKey = 'i' + ([guid]::NewGuid().ToString('N').Substring(0,16))
$soundUploaded = $false
$backgroundKey = 'i' + ([guid]::NewGuid().ToString('N').Substring(0,16))
$backgroundUploaded = $false
$test.macros = @(
    foreach ($shape in @('rounded', 'square', 'circle', 'pill', 'soft')) {
        @{
            name = ''; icon = '+'; color = 'D97706'; textColor = 'FFFFFF'; shape = $shape
            actionMode = 'sound'; sound = 'chime'; action = 'webhook'; method = 'GET'
            url = ''; body = ''; secret = ''; outline = $false; outlineColor = '00FFFF'
        }
    }
)
try {
    $test.macros[0].sound = 'invalid'
    Expect (Request '/api/config' 'POST' ($test | ConvertTo-Json -Depth 20 -Compress)) 400
    $test.macros[0].sound = 'chime'
    Expect (Request '/api/controls' 'POST' '{"brightness":101,"volume":50}') 400
    Expect (Request '/api/controls' 'POST' '{"brightness":100,"volume":100}') 200
    $maximum = (Request '/api/status').Content | ConvertFrom-Json
    if ($maximum.backlightDuty -gt 716 -or $maximum.brightness -ne 100 -or $maximum.volume -ne 100) {
        throw 'Power control range or PWM clamp is incorrect.'
    }
    Expect (Request '/api/controls' 'POST' '{"brightness":50,"volume":25}') 200
    foreach ($cols in 2..4) {
        foreach ($rows in 2..4) {
            $test.cols = $cols; $test.rows = $rows
            Expect (Request '/api/config' 'POST' ($test | ConvertTo-Json -Depth 20 -Compress)) 200
            Start-Sleep -Milliseconds 700
        }
    }
    $before = (Request '/api/status').Content | ConvertFrom-Json
    if (!$before.soundAvailable) { throw 'Device audio did not initialize.' }
    Expect (Request '/api/trigger' 'POST' '{"index":0}') 200
    Start-Sleep -Seconds 2
    $after = (Request '/api/status').Content | ConvertFrom-Json
    if ($after.audioTones -le $before.audioTones -or $after.audioBytes -le $before.audioBytes) {
        throw 'Sound trigger did not write I2S samples.'
    }
    $wav = [byte[]]::new(44 + 22050 * 2 * 4)
    [Text.Encoding]::ASCII.GetBytes('RIFF').CopyTo($wav,0)
    [BitConverter]::GetBytes([uint32]($wav.Length-8)).CopyTo($wav,4)
    [Text.Encoding]::ASCII.GetBytes('WAVEfmt ').CopyTo($wav,8)
    [BitConverter]::GetBytes([uint32]16).CopyTo($wav,16)
    [BitConverter]::GetBytes([uint16]1).CopyTo($wav,20)
    [BitConverter]::GetBytes([uint16]1).CopyTo($wav,22)
    [BitConverter]::GetBytes([uint32]22050).CopyTo($wav,24)
    [BitConverter]::GetBytes([uint32]44100).CopyTo($wav,28)
    [BitConverter]::GetBytes([uint16]2).CopyTo($wav,32)
    [BitConverter]::GetBytes([uint16]16).CopyTo($wav,34)
    [Text.Encoding]::ASCII.GetBytes('data').CopyTo($wav,36)
    [BitConverter]::GetBytes([uint32]($wav.Length-44)).CopyTo($wav,40)
    for ($i=44; $i -lt $wav.Length; $i+=2) {
        $sample = [int16](32767 * [Math]::Sin(2 * [Math]::PI * 440 * (($i-44)/2) / 22050))
        [BitConverter]::GetBytes($sample).CopyTo($wav,$i)
    }
    Expect (Request "/api/sound?key=$soundKey" 'POST' ([byte[]]::new(46)) $headers 'audio/wav') 400
    Expect (Request "/api/sound?key=$soundKey" 'POST' $wav $headers 'audio/wav') 200
    $soundUploaded = $true
    $test.theme = 'custom'; $test.background = '123456'
    $test.macros[0].sound = 'file'; $test.macros[0].soundFile = $soundKey
    Expect (Request '/api/config' 'POST' ($test | ConvertTo-Json -Depth 20 -Compress)) 200
    Expect (Request "/api/sound?key=$soundKey" 'DELETE') 400
    $before = (Request '/api/status').Content | ConvertFrom-Json
    Expect (Request '/api/trigger' 'POST' '{"index":0}') 200
    $busy = 0
    for ($i=0; $i -lt 16; $i++) {
        $response = Request '/api/trigger' 'POST' '{"index":0}'
        if ($response.StatusCode -eq 429) { $busy++ } else { Expect $response 200 }
    }
    if ($busy -eq 0) { throw 'Rapid sound taps were not throttled.' }
    Start-Sleep -Seconds 5
    $after = (Request '/api/status').Content | ConvertFrom-Json
    if ($after.uptimeMs -le $before.uptimeMs -or $after.audioBytes -le $before.audioBytes -or
        $after.audioBusy -or $after.audioPeak -gt 4915 -or $after.audioPeak -lt 4000) {
        throw 'WAV playback, repeat protection, power scaling, or uptime check failed.'
    }
    $background = [byte[]]::new(480 * 480 * 2)
    for ($i=1; $i -lt $background.Length; $i+=2) { $background[$i] = 248 }
    Expect (Request "/api/icon?key=$backgroundKey" 'POST' $background $headers 'application/octet-stream') 200
    $backgroundUploaded = $true
    $test.backgroundImage = $backgroundKey
    Expect (Request '/api/config' 'POST' ($test | ConvertTo-Json -Depth 20 -Compress)) 200
    Start-Sleep -Seconds 1
    $image = Request "/api/icon?key=$backgroundKey&size=480&format=rgb565"
    Expect $image 200
    if ($image.RawContentLength -ne 460800) { throw 'Background dimensions or payload are incorrect.' }
    Expect (Request "/api/background?key=$backgroundKey" 'DELETE') 400
    if ($Companion) {
        if (!$CompanionToken) { throw 'Provide CompanionToken with Companion.' }
        $pcToken = [PSCredential]::new('unused',$CompanionToken).GetNetworkCredential().Password
        $check = @{url=$Companion.GetLeftPart([UriPartial]::Authority)+'/status';token=$pcToken} | ConvertTo-Json -Compress
        Expect (Request '/api/companion' 'POST' $check) 200
        $test.macros[0].actionMode = 'pc_macro'; $test.macros[0].sound = 'none'
        $test.macros[0].method = 'POST'; $test.macros[0].action = 'http'
        $test.macros[0].url = $Companion.GetLeftPart([UriPartial]::Authority)+'/run'
        $test.macros[0].secret = $pcToken
        $test.macros[0].body = @{steps=@(1..12 | ForEach-Object {@{type='wait';value=0}})} | ConvertTo-Json -Depth 5 -Compress
        if ($test.macros[0].body.Length -le 256) { throw 'PC test must exceed the old body limit.' }
        Expect (Request '/api/config' 'POST' ($test | ConvertTo-Json -Depth 20 -Compress)) 200
        Expect (Request '/api/trigger' 'POST' '{"index":0}') 200
        Start-Sleep -Seconds 2
        $result = (Request '/api/companion' 'POST' $check).Content | ConvertFrom-Json
        if ($result.busy -or $result.message -ne 'PC macro completed.') { throw 'Device-to-PC sequence did not finish.' }
        $pcToken = $null
    }
} finally {
    Expect (Request '/api/config' 'POST' $original.Content) 200
    Expect (Request '/api/controls' 'POST' (@{brightness=$controls.brightness;volume=$controls.volume} | ConvertTo-Json -Compress)) 200
    if ($soundUploaded) {
        for ($i=0; $i -lt 60 -and ((Request '/api/status').Content | ConvertFrom-Json).audioBusy; $i++) {
            Start-Sleep -Milliseconds 500
        }
        Expect (Request "/api/sound?key=$soundKey" 'DELETE') 200
    }
    if ($backgroundUploaded) { Expect (Request "/api/background?key=$backgroundKey" 'DELETE') 200 }
}
if ((Request '/api/config').Content -ne $original.Content) { throw 'Original configuration was not restored.' }
'PASS: HTTPS, grids/shapes, power caps, WAV/tap protection, 480x480 background, optional device-to-PC sequence, state restoration and asset cleanup.'
