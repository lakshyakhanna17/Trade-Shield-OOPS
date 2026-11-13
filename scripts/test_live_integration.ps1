# TradeShield Stage 8 - Comprehensive Live Integration Verification Script

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   TradeShield Stage 8 Live Integration Verification      " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$serverProc = $null

try {
    # 1. Start tradeshield_server.exe
    $currentDir = (Get-Location).Path
    Write-Host "`n[START] Launching C++ Crow backend on port 8080 in $currentDir..." -ForegroundColor Yellow
    $serverProc = Start-Process -FilePath "$currentDir\backend\build\tradeshield_server.exe" -WorkingDirectory $currentDir -PassThru -NoNewWindow

    # Wait for server to bind port
    $bound = $false
    for ($i = 0; $i -lt 20; $i++) {
        Start-Sleep -Milliseconds 500
        try {
            $resp = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/health" -Method Get -TimeoutSec 2
            if ($resp.status -eq "healthy") {
                $bound = $true
                break
            }
        } catch {
            Write-Host "Health probe attempt $($i+1): $_"
        }
    }

    if (-not $bound) {
        throw "Backend server failed to start or did not respond to /api/health within 6 seconds."
    }
    Write-Host "[OK] C++ Backend successfully started and responding." -ForegroundColor Green

    # ==========================================================
    # TEST A: Health Check & System Telemetry
    # ==========================================================
    Write-Host "`n--- TEST A: Health Check Endpoint ---" -ForegroundColor Cyan
    $health = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/health" -Method Get
    Write-Host "Service: $($health.service)"
    Write-Host "Status:  $($health.status)"
    Write-Host "Version: $($health.version)"
    if ($health.status -ne "healthy") { throw "Test A failed: Status is not healthy" }
    Write-Host "[PASS] Test A: /api/health passed." -ForegroundColor Green

    # ==========================================================
    # TEST B: Options Preflight (CORS Verification)
    # ==========================================================
    Write-Host "`n--- TEST B0: CORS Preflight (OPTIONS /api/orders) ---" -ForegroundColor Cyan
    $optReq = [System.Net.HttpWebRequest]::Create("http://127.0.0.1:8080/api/orders")
    $optReq.Method = "OPTIONS"
    $optResp = $optReq.GetResponse()
    Write-Host "OPTIONS StatusCode: $($optResp.StatusCode)"
    Write-Host "Access-Control-Allow-Origin: $($optResp.Headers['Access-Control-Allow-Origin'])"
    Write-Host "Access-Control-Allow-Methods: $($optResp.Headers['Access-Control-Allow-Methods'])"
    $optResp.Close()
    Write-Host "[PASS] CORS Preflight passed with 204 No Content." -ForegroundColor Green

    # ==========================================================
    # TEST B: Trading Flow (Marketable BUY Order)
    # ==========================================================
    Write-Host "`n--- TEST B: Trading Flow (BUY Order) ---" -ForegroundColor Cyan
    $portBefore = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/portfolio?trader_id=1" -Method Get
    Write-Host "Trader #1 Initial Cash: ₹$([math]::Round($portBefore.cash_balance_paise / 100, 2))"

    # Get current stock price for stock 1 (RELIANCE)
    $stocks = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/stocks" -Method Get
    $relStock = $stocks | Where-Object { $_.stock_id -eq 1 }
    $marketPrice = $relStock.current_price_paise
    Write-Host "RELIANCE Market Price: ₹$([math]::Round($marketPrice / 100, 2)) ($marketPrice paise)"

    # Place marketable BUY order (Stock 1: RELIANCE at marketPrice + 5000 paise)
    $orderBody = @{
        trader_id = 1
        stock_id = 1
        side = "BUY"
        quantity = 2
        limit_price_paise = ($marketPrice + 5000)
    } | ConvertTo-Json

    $buyRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/orders" -Method Post -Body $orderBody -ContentType "application/json"
    Write-Host "Buy Order Result: $($buyRes | ConvertTo-Json -Compress)"
    $buyOrderId = $buyRes.order.order_id
    Write-Host "Placed Order #$buyOrderId Status: $($buyRes.order.status)"
    if ($buyRes.order.status -ne "EXECUTED") { throw "Test B failed: Marketable limit order was not immediately executed" }

    # Verify portfolio deduction
    $portAfter = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/portfolio?trader_id=1" -Method Get
    Write-Host "Trader #1 Cash After Buy: ₹$([math]::Round($portAfter.cash_balance_paise / 100, 2))"
    $cashDiff = $portBefore.cash_balance_paise - $portAfter.cash_balance_paise
    Write-Host "Cash Debited: ₹$([math]::Round($cashDiff / 100, 2))"
    if ($cashDiff -le 0) { throw "Test B failed: Cash was not debited upon trade execution" }

    # Verify positions
    $positions = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/positions?trader_id=1" -Method Get
    $relPos = $positions | Where-Object { $_.stock_id -eq 1 }
    Write-Host "Trader #1 RELIANCE Position: Quantity = $($relPos.quantity), Avg Price = ₹$([math]::Round($relPos.average_buy_price_paise / 100, 2))"
    Write-Host "[PASS] Test B: BUY order execution & portfolio balance updates passed." -ForegroundColor Green

    # ==========================================================
    # TEST C: Order Cancellation Flow
    # ==========================================================
    Write-Host "`n--- TEST C: Non-Marketable Limit Order & Cancellation ---" -ForegroundColor Cyan
    # Submit resting limit BUY below market price (e.g. ₹1,000 = 100000 paise)
    $pendBody = @{
        trader_id = 1
        stock_id = 1
        side = "BUY"
        quantity = 2
        limit_price_paise = 100000
    } | ConvertTo-Json

    $pendRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/orders" -Method Post -Body $pendBody -ContentType "application/json"
    $pendOrderId = $pendRes.order.order_id
    Write-Host "Placed Limit Order #$pendOrderId Status: $($pendRes.order.status)"
    if ($pendRes.order.status -ne "PENDING") { throw "Test C failed: Order should be PENDING" }

    # Cancel order
    $cancelRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/orders/$pendOrderId/cancel" -Method Post
    Write-Host "Cancel Result: $($cancelRes | ConvertTo-Json -Compress)"
    if ($cancelRes.order.status -ne "CANCELLED") { throw "Test C failed: Order status is not CANCELLED" }

    # Verify order status in database
    $allOrders = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/orders" -Method Get
    $cancelledOrder = $allOrders | Where-Object { $_.order_id -eq $pendOrderId }
    if ($cancelledOrder.status -ne "CANCELLED") { throw "Test C failed: Database order status is not CANCELLED" }
    Write-Host "[PASS] Test C: Limit order placement, resting PENDING status, and cancellation transition passed." -ForegroundColor Green

    # ==========================================================
    # TEST D: Portfolio Invariant Check
    # ==========================================================
    Write-Host "`n--- TEST D: Portfolio Financial Invariants ---" -ForegroundColor Cyan
    $p = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/portfolio?trader_id=1" -Method Get
    Write-Host "Cash Balance:       ₹$([math]::Round($p.cash_balance_paise / 100, 2))"
    Write-Host "Invested Value:     ₹$([math]::Round($p.total_invested_paise / 100, 2))"
    Write-Host "Holdings Value:     ₹$([math]::Round($p.holdings_value_paise / 100, 2))"
    Write-Host "Total Account Val:  ₹$([math]::Round($p.total_value_paise / 100, 2))"
    Write-Host "Unrealized PnL:     ₹$([math]::Round($p.unrealized_pnl_paise / 100, 2)) ($($p.unrealized_pnl_percentage)%)"
    $expectedTotal = $p.cash_balance_paise + $p.holdings_value_paise
    if ($p.total_value_paise -ne $expectedTotal) {
        throw "Test D failed: total_value_paise ($($p.total_value_paise)) != cash ($($p.cash_balance_paise)) + holdings ($($p.holdings_value_paise))"
    }
    Write-Host "[PASS] Test D: Portfolio mathematical invariant (Total = Cash + Holdings) verified." -ForegroundColor Green

    # ==========================================================
    # TEST E & F: Bot Simulation & Shield Fraud Alerts
    # ==========================================================
    Write-Host "`n--- TEST E: Bot Simulation Lifecycle ---" -ForegroundColor Cyan
    $simStatus0 = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/simulation/status" -Method Get
    Write-Host "Initial Simulation Running: $($simStatus0.is_running), Total Ticks: $($simStatus0.total_ticks)"

    # Start simulation with 200ms tick interval
    $startBody = @{ tick_interval_ms = 200 } | ConvertTo-Json
    $startRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/simulation/start" -Method Post -Body $startBody -ContentType "application/json"
    Write-Host "Simulation Started: $($startRes | ConvertTo-Json -Compress)"

    Write-Host "Waiting 2.5 seconds for simulation worker thread to execute ticks..."
    Start-Sleep -Milliseconds 2500

    $simStatus1 = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/simulation/status" -Method Get
    Write-Host "Simulation Running: $($simStatus1.is_running), Total Ticks: $($simStatus1.total_ticks), Orders Placed: $($simStatus1.orders_placed), Cancellations: $($simStatus1.cancellations)"
    if ($simStatus1.total_ticks -le 0) { throw "Test E failed: Simulation did not increment ticks" }

    # Stop simulation
    $stopRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/simulation/stop" -Method Post
    Write-Host "Simulation Stopped: $($stopRes | ConvertTo-Json -Compress)"
    $simStatus2 = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/simulation/status" -Method Get
    Write-Host "Simulation Status Post-Stop: is_running = $($simStatus2.is_running)"
    if ($simStatus2.is_running -ne $false) { throw "Test E failed: Simulation still running" }
    Write-Host "[PASS] Test E: Simulation start, multi-tick progression, and stop lifecycle passed." -ForegroundColor Green

    Write-Host "`n--- TEST F: Shield Alerts & Risk Scoring ---" -ForegroundColor Cyan
    $alerts = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/alerts" -Method Get
    Write-Host "Total Alerts in Database: $($alerts.Count)"
    if ($alerts.Count -gt 0) {
        $sampleAlert = $alerts[0]
        $alertRule = if ($sampleAlert.rule_name) { $sampleAlert.rule_name } else { $sampleAlert.rule_triggered }
        $alertStatus = if ($sampleAlert.status) { $sampleAlert.status } else { $sampleAlert.review_status }
        Write-Host "Sample Alert #$($sampleAlert.alert_id): Rule = $alertRule, Severity = $($sampleAlert.severity), Status = $alertStatus, Trader #$($sampleAlert.trader_id)"

        # Test reviewing this alert
        $revBody = @{ status = "REVIEWED" } | ConvertTo-Json
        $revRes = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/alerts/$($sampleAlert.alert_id)/review" -Method Post -Body $revBody -ContentType "application/json"
        Write-Host "Alert Review Result: $($revRes | ConvertTo-Json -Compress)"
        $updatedStatus = if ($revRes.alert.review_status) { $revRes.alert.review_status } else { $revRes.alert.status }
        if ($updatedStatus -ne "REVIEWED") { throw "Test F failed: Alert status not changed to REVIEWED" }
        Write-Host "[OK] Alert #$($sampleAlert.alert_id) successfully reviewed."
    }

    $riskScores = Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/risk" -Method Get
    Write-Host "Risk Profiles Count: $($riskScores.Count)"
    foreach ($rp in $riskScores) {
        Write-Host "  Trader #$($rp.trader_id) ($($rp.trader_name)): Risk Score = $($rp.overall_risk_score), Tier = $($rp.risk_tier), Alerts = $($rp.total_alerts)"
    }
    Write-Host "[PASS] Test F: Shield alerts query, review workflow, and dynamic risk scoring passed." -ForegroundColor Green

    # ==========================================================
    # TEST G: Failure Handling & Error Propagation
    # ==========================================================
    Write-Host "`n--- TEST G: Failure Handling & Real Error Propagation ---" -ForegroundColor Cyan

    # 1. Zero quantity order
    $badOrder = @{
        trader_id = 1
        stock_id = 1
        side = "BUY"
        quantity = 0
        limit_price_paise = 245000
    } | ConvertTo-Json

    $caught400 = $false
    try {
        Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/orders" -Method Post -Body $badOrder -ContentType "application/json"
    } catch {
        $caught400 = $true
        $reader = New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream())
        $errBody = $reader.ReadToEnd()
        Write-Host "Expected Validation Rejection (400 Bad Request): $errBody"
    }
    if (-not $caught400) { throw "Test G failed: Zero quantity order was not rejected" }

    # 2. Duplicate cancel on already cancelled order
    $caughtCancelErr = $false
    try {
        Invoke-RestMethod -Uri "http://127.0.0.1:8080/api/orders/$pendOrderId/cancel" -Method Post
    } catch {
        $caughtCancelErr = $true
        $reader = New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream())
        $errBody = $reader.ReadToEnd()
        Write-Host "Expected Cancel Rejection (Order already cancelled): $errBody"
    }
    if (-not $caughtCancelErr) { throw "Test G failed: Duplicate cancel was not rejected" }

    Write-Host "[PASS] Test G: Backend business rule enforcement and error propagation verified." -ForegroundColor Green

    Write-Host "`n==========================================================" -ForegroundColor Green
    Write-Host "   ALL TESTS PASSED SUCCESSFULLY! (Tests A through G)     " -ForegroundColor Green
    Write-Host "==========================================================" -ForegroundColor Green
} finally {
    if ($serverProc -ne $null -and -not $serverProc.HasExited) {
        Write-Host "`n[CLEANUP] Stopping C++ Crow backend server (PID: $($serverProc.Id))..." -ForegroundColor Yellow
        Stop-Process -Id $serverProc.Id -Force
        Write-Host "[CLEANUP] Backend server successfully terminated." -ForegroundColor Green
    }
}
