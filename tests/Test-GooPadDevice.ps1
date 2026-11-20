param(
    [Parameter(Mandatory)][uri]$Device,
    [Parameter(Mandatory)][pscredential]$Credential
)

$ErrorActionPreference = 'Stop'
if ($Device.Scheme -ne 'https') { throw 'Use the HTTPS device address.' }
$base = $Device.GetLeftPart([System.UriPartial]::Authority)
$token = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes(
    "$($Credential.UserName):$($Credential.GetNetworkCredential().Password)"))
$headers = @{ Authorization = "Basic $token" }
function Request($Path, $Method = 'GET', $Body = $null, $Headers = $headers) {
    $options = @{
        Uri = "$base$Path"; Method = $Method; Headers = $Headers
        SkipCertificateCheck = $true; SkipHttpErrorCheck = $true; TimeoutSec = 30
    }
    if ($null -ne $Body) { $options.Body = $Body; $options.ContentType = 'application/json' }
    Invoke-WebRequest @options
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
$test.macros = @(
    foreach ($shape in @('rounded', 'square', 'circle', 'pill', 'soft')) {
        @{
            name = ''; icon = '+'; color = 'D97706'; textColor = 'FFFFFF'; shape = $shape
            actionMode = 'sound'; sound = 'chime'; action = 'webhook'; method = 'GET'
            url = ''; body = ''; secret = ''
        }
    }
)
try {
    $test.macros[0].sound = 'invalid'
    Expect (Request '/api/config' 'POST' ($test | ConvertTo-Json -Depth 20 -Compress)) 400
    $test.macros[0].sound = 'chime'
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
} finally {
    Expect (Request '/api/config' 'POST' $original.Content) 200
}
if ((Request '/api/config').Content -ne $original.Content) { throw 'Original configuration was not restored.' }
'PASS: HTTPS authentication, Font Awesome JSON, config rejection, nine grids/five shapes, I2S sound, config restoration.'
