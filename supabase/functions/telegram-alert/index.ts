// Supabase Edge Function: telegram-alert
// Receives Database Webhook on INSERT to air_quality_logs and notifies a Telegram Bot on warnings.
// Also supports scheduled tasks: ?action=check_status (device watchdog) and ?action=daily_report (7 AM summary).

import { createClient } from "https://esm.sh/@supabase/supabase-js@2.39.7";

const TELEGRAM_BOT_TOKEN = Deno.env.get("TELEGRAM_BOT_TOKEN");
const TELEGRAM_CHAT_ID = Deno.env.get("TELEGRAM_CHAT_ID");

// Warning Thresholds loaded from environment variables with safe fallbacks
const MQ135_ALERT_THRESHOLD = parseFloat(Deno.env.get("MQ135_ALERT_THRESHOLD") || "1000");
const GAS_INDEX_ALERT_THRESHOLD = parseFloat(Deno.env.get("GAS_INDEX_ALERT_THRESHOLD") || "100");
const DUST_ALERT_THRESHOLD = parseFloat(Deno.env.get("DUST_ALERT_THRESHOLD") || "150");
const TEMP_ALERT_THRESHOLD = parseFloat(Deno.env.get("TEMP_ALERT_THRESHOLD") || "40");
const HUMIDITY_ALERT_THRESHOLD = parseFloat(Deno.env.get("HUMIDITY_ALERT_THRESHOLD") || "85");

// Initialize Supabase client
const SUPABASE_URL = Deno.env.get("SUPABASE_URL") || "";
const SUPABASE_SERVICE_ROLE_KEY = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") || "";
const supabase = createClient(SUPABASE_URL, SUPABASE_SERVICE_ROLE_KEY);

// Interface representing the database row structure in the webhook record
interface AirQualityRecord {
  id: number;
  created_at: string;
  temperature: number | null;
  humidity: number | null;
  mq135_ppm: number | null;
  mq135_v: number | null;
  gas_index: number | null;
  dust_density: number | null;
  status: number | null;
  warning: boolean | null;
}

interface WebhookPayload {
  type: "INSERT" | "UPDATE" | "DELETE";
  table: string;
  schema: string;
  record: AirQualityRecord;
  old_record: AirQualityRecord | null;
}

// Helper function to send message to Telegram
async function sendTelegramMessage(text: string): Promise<Response | null> {
  if (!TELEGRAM_BOT_TOKEN || !TELEGRAM_CHAT_ID) {
    console.error("TELEGRAM_BOT_TOKEN or TELEGRAM_CHAT_ID is missing.");
    return null;
  }
  const telegramUrl = `https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage`;
  const response = await fetch(telegramUrl, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      chat_id: TELEGRAM_CHAT_ID,
      text: text,
    }),
  });
  return response;
}

Deno.serve(async (req: Request) => {
  try {
    // Only accept POST requests
    if (req.method !== "POST") {
      return new Response(JSON.stringify({ error: "Method not allowed" }), {
        status: 405,
        headers: { "Content-Type": "application/json" },
      });
    }

    // Check URL parameters for scheduled actions
    const url = new URL(req.url);
    const action = url.searchParams.get("action");

    // ==========================================
    // ACTION: Watchdog Connection Check
    // ==========================================
    if (action === "check_status") {
      console.log("Running Watchdog Connection Check...");
      const { data, error } = await supabase
        .from("air_quality_logs")
        .select("created_at")
        .order("created_at", { ascending: false })
        .limit(1);

      if (error) {
        throw new Error(`Failed to query latest logs: ${error.message}`);
      }

      if (!data || data.length === 0) {
        return new Response(JSON.stringify({ alert: false, message: "No data available in logs table" }), {
          status: 200,
          headers: { "Content-Type": "application/json" },
        });
      }

      const latestTime = new Date(data[0].created_at);
      const diffMs = new Date().getTime() - latestTime.getTime();
      const diffMins = diffMs / (1000 * 60);

      console.log(`Latest log recorded at ${latestTime.toISOString()} (${diffMins.toFixed(1)} mins ago)`);

      // Trigger alert if offline duration is between 15 and 20 minutes (triggers exactly once in a 5-minute cron check window)
      if (diffMins >= 15 && diffMins < 20) {
        const localTimeStr = new Intl.DateTimeFormat("vi-VN", {
          timeZone: "Asia/Ho_Chi_Minh",
          year: "numeric",
          month: "2-digit",
          day: "2-digit",
          hour: "2-digit",
          minute: "2-digit",
          second: "2-digit",
        }).format(latestTime);

        const disconnectMsg = `⚠️ CANH BAO MAT KET NOI
Thiet bi do chat luong khong khi da mat ket noi qua 15 phut!
Thoi gian ghi nhan cuoi cung: ${localTimeStr}`;

        const response = await sendTelegramMessage(disconnectMsg);
        if (response && response.ok) {
          return new Response(JSON.stringify({ alert: true, telegram: "sent", status: "offline_warning" }), {
            status: 200,
            headers: { "Content-Type": "application/json" },
          });
        }
      }

      return new Response(JSON.stringify({ alert: false, message: "Device is online or alert already sent" }), {
        status: 200,
        headers: { "Content-Type": "application/json" },
      });
    }

    // ==========================================
    // ACTION: Daily Air Quality Summary (7:00 AM)
    // ==========================================
    if (action === "daily_report") {
      console.log("Generating Daily Air Quality Summary...");
      const last24h = new Date(new Date().getTime() - 24 * 60 * 60 * 1000).toISOString();
      const { data: stats, error: statsError } = await supabase
        .from("air_quality_logs")
        .select("temperature, humidity, gas_index, mq135_ppm, dust_density")
        .gte("created_at", last24h);

      if (statsError) {
        throw new Error(`Failed to query daily statistics: ${statsError.message}`);
      }

      let avgTemp = 0, avgHum = 0, avgGasIndex = 0, avgMq = 0, avgDust = 0;
      let countGasIndex = 0;
      let countTemp = 0, countHum = 0, countMq = 0, countDust = 0;

      if (stats && stats.length > 0) {
        for (const row of stats) {
          if (row.temperature !== null) { avgTemp += row.temperature; countTemp++; }
          if (row.humidity !== null) { avgHum += row.humidity; countHum++; }
          if (row.gas_index !== null) { avgGasIndex += row.gas_index; countGasIndex++; }
          if (row.mq135_ppm !== null) { avgMq += row.mq135_ppm; countMq++; }
          if (row.dust_density !== null) { avgDust += row.dust_density; countDust++; }
        }
      }

      const reportTemp = countTemp > 0 ? `${(avgTemp / countTemp).toFixed(1)} oC` : "...";
      const reportHum = countHum > 0 ? `${(avgHum / countHum).toFixed(1)} %` : "...";
      const reportMq = countGasIndex > 0
        ? `${(avgGasIndex / countGasIndex).toFixed(1)} / 100 (chi so tuong doi)`
        : countMq > 0 ? `${(avgMq / countMq).toFixed(1)} (uoc luong cu)` : "...";
      const reportDust = countDust > 0 ? `${(avgDust / countDust).toFixed(1)} ug/m3` : "...";

      const formattedTime = new Intl.DateTimeFormat("vi-VN", {
        timeZone: "Asia/Ho_Chi_Minh",
        year: "numeric",
        month: "2-digit",
        day: "2-digit",
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
      }).format(new Date());

      const reportMsg = `Bao cao chat luong khong khi hang ngay (7:00 AM)
Trung binh 24h qua:
Nhiet do: ${reportTemp}
Do am: ${reportHum}
MQ135: ${reportMq}
Bui: ${reportDust}
Thoi gian bao cao: ${formattedTime}`;

      const response = await sendTelegramMessage(reportMsg);
      if (response && response.ok) {
        return new Response(JSON.stringify({ alert: true, telegram: "sent", status: "daily_report" }), {
          status: 200,
          headers: { "Content-Type": "application/json" },
        });
      }
      throw new Error("Failed to send daily report to Telegram");
    }

    // ==========================================
    // ACTION: Insert Webhook Alert check (default)
    // ==========================================
    const payload: WebhookPayload = await req.json();
    const record = payload.record;

    if (!record) {
      return new Response(JSON.stringify({ error: "No record found in payload" }), {
        status: 400,
        headers: { "Content-Type": "application/json" },
      });
    }

    console.log(`Processing air quality log ID: ${record.id}`);

    // Check individual threshold warning conditions based on environment variables
    const isTempHigh = record.temperature !== null && record.temperature > TEMP_ALERT_THRESHOLD;
    const isHumHigh = record.humidity !== null && record.humidity > HUMIDITY_ALERT_THRESHOLD;
    // Prefer the relative gas index used by Method A. Keep the legacy PPM
    // fallback for old rows created before the schema/firmware migration.
    const isGasHigh = record.gas_index !== null
      ? record.gas_index > GAS_INDEX_ALERT_THRESHOLD
      : record.mq135_ppm !== null && record.mq135_ppm > MQ135_ALERT_THRESHOLD;
    const isDustHigh = record.dust_density !== null && record.dust_density > DUST_ALERT_THRESHOLD;
    
    // Status warning: 1 = SENSOR_READ_ERROR, 2 = SENSOR_TIMEOUT, 3 = OFFLINE_CACHED
    // We ignore OFFLINE_CACHED (3) to avoid spamming alerts for successfully recovered offline logs.
    const SENSOR_OK = 0;
    const OFFLINE_CACHED = 3;
    const isSensorError = record.status !== null && record.status !== SENSOR_OK && record.status !== OFFLINE_CACHED;

    // Determine if warning is triggered overall
    const isWarningTriggered = isTempHigh || isHumHigh || isGasHigh || isDustHigh || isSensorError;

    if (isWarningTriggered) {
      console.log(`Warning conditions met for record ID ${record.id}. Preparing Telegram notification...`);
      
      let statusStr = "XAU";
      const reasons: string[] = [];

      if (isSensorError) {
        statusStr = "LOI CAM BIEN";
        reasons.push("Loi cam bien / loi he thong");
      } else {
        if (isTempHigh) reasons.push(`Nhiet do cao (${record.temperature}°C > ${TEMP_ALERT_THRESHOLD}°C)`);
        if (isHumHigh) reasons.push(`Do am cao (${record.humidity}% > ${HUMIDITY_ALERT_THRESHOLD}%)`);
        if (isGasHigh) {
          if (record.gas_index !== null) {
            reasons.push(`Chi so khi cao (${record.gas_index.toFixed(1)} > ${GAS_INDEX_ALERT_THRESHOLD})`);
          } else {
            reasons.push(`Khi gas uoc luong cao (${record.mq135_ppm} > ${MQ135_ALERT_THRESHOLD})`);
          }
        }
        if (isDustHigh) reasons.push(`Bui min cao (${record.dust_density} ug/m3 > ${DUST_ALERT_THRESHOLD} ug/m3)`);
      }

      // Convert created_at to local Vietnam time (+07:00)
      const timeUTC = new Date(record.created_at || new Date().toISOString());
      const formatter = new Intl.DateTimeFormat("vi-VN", {
        timeZone: "Asia/Ho_Chi_Minh",
        year: "numeric",
        month: "2-digit",
        day: "2-digit",
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit",
      });
      const formattedTime = formatter.format(timeUTC);

      // Formatting fields for user's template
      const tempVal = record.temperature !== null ? `${record.temperature.toFixed(1)} oC` : "...";
      const humVal = record.humidity !== null ? `${record.humidity.toFixed(1)} %` : "...";
      
      let mqVal = "...";
      if (record.gas_index !== null) {
        mqVal = `${record.gas_index.toFixed(1)} / 100 (chi so tuong doi)`;
      } else if (record.mq135_ppm !== null) {
        mqVal = `${record.mq135_ppm.toFixed(1)} (uoc luong cu)`;
      } else if (record.mq135_v !== null) {
        mqVal = `${record.mq135_v.toFixed(2)} V`;
      }

      const dustVal = record.dust_density !== null ? `${record.dust_density.toFixed(1)} ug/m3` : "...";

      // Build message exactly matching user's requested layout:
      // Canh bao chat luong khong khi
      // Trang thai: XAU
      // Nhiet do: ...
      // Do am: ...
      // MQ135: ...
      // Bui: ...
      // Thoi gian: ...
      let telegramMessage = `Canh bao chat luong khong khi
Trang thai: ${statusStr}
Nhiet do: ${tempVal}
Do am: ${humVal}
MQ135: ${mqVal}
Bui: ${dustVal}
Thoi gian: ${formattedTime}`;

      // Append detailed reasons if any
      if (reasons.length > 0) {
        telegramMessage += `\nChi tiet: ${reasons.join(", ")}`;
      }

      const response = await sendTelegramMessage(telegramMessage);

      if (!response || !response.ok) {
        const responseText = response ? await response.text() : "Network error";
        console.error(`Telegram sendMessage failed: ${responseText}`);
        return new Response(JSON.stringify({ error: `Telegram Bot API error`, details: responseText }), {
          status: 502,
          headers: { "Content-Type": "application/json" },
        });
      }

      console.log("Telegram notification sent successfully.");
      return new Response(JSON.stringify({ alert: true, telegram: "sent" }), {
        status: 200,
        headers: { "Content-Type": "application/json" },
      });
    }

    console.log("Record analyzed. Warning conditions not met. No notification sent.");
    return new Response(JSON.stringify({ alert: false, message: "No alert condition matched" }), {
      status: 200,
      headers: { "Content-Type": "application/json" },
    });
  } catch (error) {
    const errorMsg = error instanceof Error ? error.message : String(error);
    console.error("Exception occurred in Edge Function:", error);
    return new Response(JSON.stringify({ error: errorMsg }), {
      status: 500,
      headers: { "Content-Type": "application/json" },
    });
  }
});
