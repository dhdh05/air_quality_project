# 1. Parse secrets.h to extract Supabase credentials
$secretsPath = Join-Path $PSScriptRoot "..\firmware\include\secrets.h"
if (-not (Test-Path $secretsPath)) {
    Write-Error "Could not find secrets.h at $secretsPath"
    exit 1
}

$content = Get-Content $secretsPath -Raw
$urlMatch = [regex]::Match($content, '#define\s+SUPABASE_URL\s+"([^"]+)"')
$keyMatch = [regex]::Match($content, '#define\s+SUPABASE_KEY\s+"([^"]+)"')

if (-not $urlMatch.Success -or -not $keyMatch.Success) {
    Write-Error "Could not extract credentials from secrets.h"
    exit 1
}

$supabaseUrl = $urlMatch.Groups[1].Value
$supabaseKey = $keyMatch.Groups[1].Value

Write-Host "Extracted Supabase URL: $supabaseUrl"

# 2. Generate records (144 records, 1 every 10 mins)
$records = @()
$now = [DateTime]::UtcNow

Write-Host "Simulating 24 hours of air quality data..."
for ($i = 0; $i -lt 144; $i++) {
    $recordTime = $now.AddMinutes(-10 * $i)
    # Format ISO 8601 with UTC Z designator
    $timeStr = $recordTime.ToString("yyyy-MM-ddTHH:mm:ssZ")
    
    $status = 0
    $warning = $false
    
    # Inject occasional sensor errors (every 50 records)
    if ($i -eq 15 -or $i -eq 90) {
        $status = 1  # SENSOR_READ_ERROR
        $temp = $null
        $hum = $null
        $mq135_ppm = $null
        $mq135_v = $null
        $dust = $null
    } else {
        # Normal Temperature: 28C to 33C
        $temp = [Math]::Round(30.5 + 2.5 * [Math]::Sin($i * 0.15) + (Get-Random -Min -0.3 -Max 0.3), 1)
        # Normal Humidity: 55% to 70%
        $hum = [Math]::Round(62.0 - 7.0 * [Math]::Sin($i * 0.15) + (Get-Random -Min -0.5 -Max 0.5), 1)
        
        # MQ135 values
        if ($i -eq 50 -or $i -eq 110) {  # Gas leak warning events
            $mq135_v = [Math]::Round((Get-Random -Min 2.80 -Max 2.95), 2)
            $mq135_ppm = [Math]::Round((Get-Random -Min 1050.0 -Max 1200.0), 1)
        } else {  # Normal values
            $mq135_v = [Math]::Round(1.2 + 0.3 * [Math]::Sin($i * 0.1) + (Get-Random -Min -0.05 -Max 0.05), 2)
            $mq135_ppm = [Math]::Round(120.0 + 80.0 * [Math]::Sin($i * 0.1) + (Get-Random -Min -10.0 -Max 10.0), 1)
        }
        
        # GP2Y dust values
        if ($i -eq 80) {  # Dust storm event
            $dust = [Math]::Round((Get-Random -Min 155.0 -Max 175.0), 1)
        } else {  # Normal dust levels
            $dust = [Math]::Round(35.0 + 15.0 * [Math]::Sin($i * 0.08) + (Get-Random -Min -5.0 -Max 5.0), 1)
        }
        
        # Determine warning flag based on configured thresholds
        $warning = ($temp -gt 40.0) -or ($hum -gt 85.0) -or ($mq135_ppm -gt 1000.0) -or ($dust -and $dust -gt 150.0)
    }
    
    $records += ,[PSCustomObject]@{
        created_at = $timeStr
        temperature = $temp
        humidity = $hum
        mq135_ppm = $mq135_ppm
        mq135_v = $mq135_v
        dust_density = $dust
        status = $status
        warning = $warning
    }
}

# 3. Save local mock datasets to data/
$dataDir = Join-Path $PSScriptRoot "..\data"
if (-not (Test-Path $dataDir)) {
    New-Item -ItemType Directory -Force -Path $dataDir | Out-Null
}

# Save as JSON
$jsonPath = Join-Path $dataDir "mock_records.json"
$records | ConvertTo-Json -Depth 5 | Out-File $jsonPath -Encoding utf8

# Save as CSV
$csvPath = Join-Path $dataDir "air_quality_history.csv"
$csvContent = "created_at,temperature,humidity,mq135_ppm,mq135_v,dust_density,status,warning`n"
foreach ($r in $records) {
    $csvContent += "$($r.created_at),$($r.temperature),$($r.humidity),$($r.mq135_ppm),$($r.mq135_v),$($r.dust_density),$($r.status),$($r.warning)`n"
}
$csvContent | Out-File $csvPath -Encoding utf8

Write-Host "[OK] Generated $($records.Count) records."
Write-Host "[OK] Local JSON dataset saved to: $jsonPath"
Write-Host "[OK] Local CSV dataset saved to: $csvPath"

# 4. Upload to Supabase
Write-Host ""
Write-Host "Uploading mock records to Supabase DB..."
$successCount = 0

$headers = @{
    "Content-Type" = "application/json"
    "apikey" = $supabaseKey
    "Authorization" = "Bearer $supabaseKey"
}

# Reverse array to insert the oldest records first (so chronological order is preserved)
[array]::Reverse($records)

for ($idx = 0; $idx -lt $records.Count; $idx++) {
    $r = $records[$idx]
    $body = $r | ConvertTo-Json -Depth 5
    try {
        $response = Invoke-RestMethod -Method Post -Uri $supabaseUrl -Headers $headers -Body $body
        $successCount++
    } catch {
        Write-Host "[ERROR] Failed to upload record $($idx) - $_"
    }
}

Write-Host ""
Write-Host "======================================================="
Write-Host "  UPLOAD COMPLETE: $successCount/$($records.Count) records uploaded!"
Write-Host "======================================================="
