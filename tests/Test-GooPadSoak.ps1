param(
    [Parameter(Mandatory)][uri]$Device,
    [Parameter(Mandatory)][pscredential]$Credential,
    [Parameter(Mandatory)][string]$CertificateSha256,
    [ValidateRange(1, 604800)][int]$DurationSeconds = 300,
    [ValidateRange(1, 3600)][int]$IntervalSeconds = 15,
    [ValidateRange(1, 120)][int]$RequestTimeoutSeconds = 10,
    [string]$ReportPath
)

$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSVersion.Major -lt 7) { throw 'PowerShell 7 or newer is required.' }
if ($Device.Scheme -ne 'https' -or $Device.UserInfo -or $Device.AbsolutePath -ne '/' -or $Device.Query -or $Device.Fragment) {
    throw 'Device must be an HTTPS origin without credentials, path, query, or fragment.'
}
$pin = ($CertificateSha256 -replace '[:\s-]', '').ToUpperInvariant()
if ($pin -cnotmatch '^[0-9A-F]{64}$') { throw 'CertificateSha256 must be a SHA-256 certificate fingerprint.' }

if (-not ('GooPadSoakTransport' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Net.Security;
using System.Security.Cryptography;
using System.Text;

public sealed class GooPadSoakTransport : IDisposable {
    private readonly HttpClient client;
    public GooPadSoakTransport(string fingerprint, string username, string password, int timeout) {
        byte[] expected = Convert.FromHexString(fingerprint);
        var handler = new HttpClientHandler { AllowAutoRedirect = false, UseCookies = false, UseProxy = false };
        handler.ServerCertificateCustomValidationCallback = (request, certificate, chain, errors) =>
            certificate != null &&
            (errors & (SslPolicyErrors.RemoteCertificateNameMismatch | SslPolicyErrors.RemoteCertificateNotAvailable)) == 0 &&
            CryptographicOperations.FixedTimeEquals(SHA256.HashData(certificate.RawData), expected);
        client = new HttpClient(handler) { Timeout = TimeSpan.FromSeconds(timeout), MaxResponseContentBufferSize = 131072 };
        string basic = Convert.ToBase64String(Encoding.UTF8.GetBytes(username + ":" + password));
        client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Basic", basic);
    }
    public HttpResponseMessage Get(string url) => client.GetAsync(url).GetAwaiter().GetResult();
    public void Dispose() => client.Dispose();
}
'@
}

$base = $Device.GetLeftPart([System.UriPartial]::Authority)
$writer = $null
$client = $null
$clock = [Diagnostics.Stopwatch]::StartNew()
$samples = 0; $errors = 0; $warnings = 0; $reboots = 0
$minInternal = $null; $minLargest = $null; $minPsram = $null
$firstInternal = $null; $lastInternal = $null; $firstPsram = $null; $lastPsram = $null
$previousUptime = $null; $previousEvents = $null; $initialSd = $null

function Write-Record([hashtable]$Record) {
    $Record.elapsedSeconds = [math]::Round($clock.Elapsed.TotalSeconds, 3)
    $Record.timestampUtc = [DateTimeOffset]::UtcNow.ToString('o')
    if ($writer) { $writer.WriteLine(($Record | ConvertTo-Json -Compress -Depth 5)); $writer.Flush() }
}
function Report-Problem([string]$Severity, [string]$Code, [string]$Detail) {
    if ($Severity -eq 'error') { $script:errors++ } else { $script:warnings++ }
    Write-Host ("[{0:N1}s] {1}: {2} {3}" -f $clock.Elapsed.TotalSeconds, $Severity, $Code, $Detail)
    Write-Record @{type=$Severity; code=$Code; detail=$Detail}
}
function Read-Endpoint([string]$Path) {
    try {
        $response = $client.Get("$base$Path")
        try {
            if ([int]$response.StatusCode -ne 200) {
                Report-Problem 'error' 'http' "$Path returned HTTP $([int]$response.StatusCode)"
                return $null
            }
            $content = $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
            try { return ConvertFrom-Json -InputObject $content -AsHashtable -NoEnumerate -ErrorAction Stop }
            catch [System.ArgumentException], [System.InvalidOperationException], [System.Management.Automation.RuntimeException] {
                Report-Problem 'error' 'invalid-json' "$Path returned invalid JSON"
                return $null
            }
        } finally { $response.Dispose() }
    } catch [System.Net.Http.HttpRequestException], [System.Threading.Tasks.TaskCanceledException], [System.OperationCanceledException] {
        Report-Problem 'error' 'request' "$Path failed (network, timeout, or certificate validation)"
        return $null
    }
}
function Is-Number($Value) {
    return $Value -is [int] -or $Value -is [long] -or $Value -is [double] -or $Value -is [decimal]
}
function Validate-Diagnostics($Data) {
    if ($Data -isnot [System.Collections.IDictionary] -or $Data.format -ne 1 -or $Data.events -isnot [array]) { return $false }
    foreach ($key in @('uptimeSeconds', 'internalFree', 'internalLargest', 'psramFree', 'resetReason')) {
        if (!(Is-Number $Data[$key]) -or $Data[$key] -lt 0) { return $false }
    }
    foreach ($event in $Data.events) {
        if ($event -isnot [array] -or $event.Count -ne 4) { return $false }
        foreach ($value in $event) {
            if (!(Is-Number $value) -or $value -ne [math]::Truncate($value)) { return $false }
        }
        if ($event[0] -lt 0 -or $event[1] -lt 1 -or $event[1] -gt 7) { return $false }
    }
    return $true
}
function Event-Key($Event) { return "$($Event[0]),$($Event[1]),$($Event[2]),$($Event[3])" }

try {
    if ($ReportPath) {
        $stream = [IO.File]::Open($ReportPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::Read)
        $writer = [IO.StreamWriter]::new($stream, [Text.UTF8Encoding]::new($false))
    }
    $client = [GooPadSoakTransport]::new($pin, $Credential.UserName, $Credential.GetNetworkCredential().Password, $RequestTimeoutSeconds)
    Write-Host "Monitoring GooPad for $DurationSeconds seconds at $IntervalSeconds-second intervals. GET requests only."
    Write-Record @{type='start'; durationSeconds=$DurationSeconds; intervalSeconds=$IntervalSeconds}
    do {
        $poll = [Diagnostics.Stopwatch]::StartNew()
        $status = Read-Endpoint '/api/status'
        $statusLatency = $poll.Elapsed.TotalMilliseconds
        $diag = Read-Endpoint '/api/diagnostics'
        $poll.Stop()
        $diagnosticsLatency = $poll.Elapsed.TotalMilliseconds - $statusLatency
        if ($null -ne $status -and $null -ne $diag) {
            if ($status -isnot [System.Collections.IDictionary] -or $status.connected -isnot [bool] -or $status.sd -isnot [bool]) {
                Report-Problem 'error' 'status-schema' 'Status fields are missing or invalid'
            } elseif (!(Validate-Diagnostics $diag)) {
                Report-Problem 'error' 'diagnostics-schema' 'Diagnostics fields are missing or invalid'
            } else {
                $samples++
                if (!$status.connected) { Report-Problem 'warning' 'wifi' 'Device reports disconnected' }
                if ($null -eq $initialSd) { $initialSd = $status.sd }
                elseif ($initialSd -and !$status.sd) { Report-Problem 'warning' 'sd' 'Previously mounted SD card is unavailable' }
                $unicode = $status.unicode
                $validUnicode = $unicode -is [System.Collections.IDictionary] -and $unicode.available -is [bool] -and
                    (Is-Number $unicode.bitmapCacheBytes) -and (Is-Number $unicode.bitmapCachePeak) -and
                    $unicode.bitmapCacheBytes -ge 0 -and $unicode.bitmapCachePeak -ge 0
                if ($status.ContainsKey('unicode')) {
                    if (!$validUnicode) {
                        Report-Problem 'error' 'unicode-schema' 'Unicode health fields are missing or invalid'
                    } elseif (!$unicode.available) {
                        Report-Problem 'error' 'unicode-unavailable' 'Multilingual SD font resources are unavailable'
                    } elseif ($unicode.bitmapCacheBytes -gt 32768 -or $unicode.bitmapCachePeak -gt 32768) {
                        Report-Problem 'error' 'unicode-cache' 'Unicode bitmap cache exceeded its 32 KiB budget'
                    }
                }
                $styled = $status.styledFonts
                $validStyled = $styled -is [System.Collections.IDictionary] -and $styled.available -is [bool]
                $styledMetadata = 0
                if ($validStyled) {
                    foreach ($key in @('descriptorBytes', 'metadataCacheBytes', 'kerningCacheBytes',
                                       'sharedBitmapCacheBytes', 'bitmapCachePeak', 'bitmapCacheBudget')) {
                        if (!(Is-Number $styled[$key]) -or $styled[$key] -lt 0) { $validStyled = $false }
                    }
                    if ($validStyled) {
                        $styledMetadata = $styled.descriptorBytes + $styled.metadataCacheBytes + $styled.kerningCacheBytes
                    }
                }
                if ($status.ContainsKey('styledFonts')) {
                    if (!$validStyled) {
                        Report-Problem 'error' 'styled-schema' 'Styled font health fields are missing or invalid'
                    } elseif (!$styled.available) {
                        Report-Problem 'error' 'styled-unavailable' 'Styled SD font resources are unavailable'
                    } elseif ($styled.bitmapCacheBudget -ne 32768 -or
                              $styled.sharedBitmapCacheBytes -gt 32768 -or $styled.bitmapCachePeak -gt 32768) {
                        Report-Problem 'error' 'styled-cache' 'Shared font bitmap cache exceeded its 32 KiB budget'
                    } elseif ($styledMetadata -gt 131072) {
                        Report-Problem 'error' 'styled-memory' 'Styled font metadata exceeded its conservative 128 KiB ceiling'
                    }
                }
                if ($null -ne $previousUptime -and $diag.uptimeSeconds -lt $previousUptime) {
                    $reboots++
                    Report-Problem 'error' 'reboot' "Uptime decreased; reset reason $($diag.resetReason)"
                }
                $previousUptime = $diag.uptimeSeconds
                if ($null -eq $firstInternal) { $firstInternal = $diag.internalFree; $firstPsram = $diag.psramFree }
                $lastInternal = $diag.internalFree; $lastPsram = $diag.psramFree
                $minInternal = if ($null -eq $minInternal) { $diag.internalFree } else { [math]::Min($minInternal, $diag.internalFree) }
                $minLargest = if ($null -eq $minLargest) { $diag.internalLargest } else { [math]::Min($minLargest, $diag.internalLargest) }
                $minPsram = if ($null -eq $minPsram) { $diag.psramFree } else { [math]::Min($minPsram, $diag.psramFree) }
                if ($diag.internalFree -lt 24576 -or $diag.internalLargest -lt 4096) {
                    Report-Problem 'warning' 'memory' 'Internal free memory or largest block is below the firmware network threshold'
                }
                $keys = @($diag.events | ForEach-Object { Event-Key $_ })
                if ($null -ne $previousEvents) {
                    $overlap = 0
                    for ($count = [math]::Min($previousEvents.Count, $keys.Count); $count -gt 0; $count--) {
                        $same = $true
                        for ($i = 0; $i -lt $count; $i++) {
                            if ($previousEvents[$previousEvents.Count - $count + $i] -ne $keys[$i]) { $same = $false; break }
                        }
                        if ($same) { $overlap = $count; break }
                    }
                    if ($previousEvents.Count -and $keys.Count -and !$overlap) {
                        Report-Problem 'warning' 'event-gap' 'Diagnostic ring changed without overlap; events may have been missed'
                        $overlap = $keys.Count
                    }
                    for ($i = $overlap; $i -lt $diag.events.Count; $i++) {
                        $event = $diag.events[$i]
                        $type = [int]$event[1]; $result = [int]$event[2]; $detail = [int]$event[3]
                        if ($type -eq 2 -or $type -eq 5) {
                            Report-Problem 'warning' 'device-event' "type=$type result=$result detail=$detail"
                        } elseif (($type -eq 3 -or $type -eq 4) -and ($result -ne 0 -or ($detail -ne 0 -and ($detail -lt 200 -or $detail -ge 300)))) {
                            Report-Problem 'error' 'device-event' "type=$type result=$result detail=$detail"
                        } elseif ($type -eq 7 -and ($result -lt 200 -or $result -ge 300)) {
                            Report-Problem 'error' 'device-event' "type=$type result=$result detail=$detail"
                        }
                    }
                }
                $previousEvents = $keys
                $sample = @{type='sample'; uptimeSeconds=$diag.uptimeSeconds; internalFree=$diag.internalFree;
                    internalLargest=$diag.internalLargest; psramFree=$diag.psramFree; latencyMs=[math]::Round($poll.Elapsed.TotalMilliseconds);
                    statusLatencyMs=[math]::Round($statusLatency); diagnosticsLatencyMs=[math]::Round($diagnosticsLatency);
                    connected=$status.connected; sd=$status.sd}
                if ($validUnicode) {
                    $sample.unicodeAvailable = $unicode.available
                    $sample.unicodeBitmapCacheBytes = $unicode.bitmapCacheBytes
                    $sample.unicodeBitmapCachePeak = $unicode.bitmapCachePeak
                }
                if ($validStyled) {
                    $sample.styledAvailable = $styled.available
                    $sample.styledMetadataBytes = $styledMetadata
                    $sample.sharedFontBitmapCacheBytes = $styled.sharedBitmapCacheBytes
                    $sample.styledBitmapCachePeak = $styled.bitmapCachePeak
                }
                foreach ($key in @('mediaRevision', 'mediaPollMs', 'mediaSourceAgeMs')) {
                    if ($status.ContainsKey($key)) {
                        if ((Is-Number $status[$key]) -and $status[$key] -ge 0) { $sample[$key] = $status[$key] }
                        else { Report-Problem 'error' 'media-schema' 'Media telemetry is invalid' }
                    }
                }
                Write-Record $sample
                Write-Host ("sample {0}: uptime={1}s internal={2} largest={3} psram={4} latency={5}ms" -f
                    $samples, $diag.uptimeSeconds, $diag.internalFree, $diag.internalLargest, $diag.psramFree, [math]::Round($poll.Elapsed.TotalMilliseconds))
            }
        }
        $remaining = $DurationSeconds - $clock.Elapsed.TotalSeconds
        if ($remaining -gt 0) { Start-Sleep -Milliseconds ([int]([math]::Min($IntervalSeconds, $remaining) * 1000)) }
    } while ($clock.Elapsed.TotalSeconds -lt $DurationSeconds)
    if (!$samples) { Report-Problem 'error' 'no-samples' 'No valid device samples were collected' }
    Write-Record @{type='summary'; samples=$samples; errors=$errors; warnings=$warnings; reboots=$reboots;
        minimumInternalFree=$minInternal; minimumInternalLargest=$minLargest; minimumPsramFree=$minPsram;
        firstInternalFree=$firstInternal; lastInternalFree=$lastInternal; firstPsramFree=$firstPsram; lastPsramFree=$lastPsram}
    Write-Host "Summary: $samples samples, $errors errors, $warnings warnings, $reboots reboots."
    if ($samples) { Write-Host "Internal free: $firstInternal -> $lastInternal bytes (minimum $minInternal); PSRAM free: $firstPsram -> $lastPsram bytes (minimum $minPsram)." }
} finally {
    if ($client) { $client.Dispose() }
    if ($writer) { $writer.Dispose() }
}
if ($errors) { exit 1 }
