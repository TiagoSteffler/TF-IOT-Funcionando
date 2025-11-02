import { INFLUX } from '../config'

// Minimal InfluxDB v2 client for browser dev usage.
// Note: embedding token in client is insecure for production. Prefer a backend proxy.

export async function queryPin(pinNumber, { range = '-1h' } = {}) {
  const flux = `from(bucket: \"${INFLUX.bucket}\")\n  |> range(start: ${range})\n  |> filter(fn: (r) => r[\"pin\"] == \"${pinNumber}\")\n  |> keep(columns: [\"_time\", \"_value\", \"_field\"])\n  |> sort(columns:[\"_time\"])`;

  const url = `${INFLUX.url}/api/v2/query?org=${encodeURIComponent(INFLUX.org)}`;

  const res = await fetch(url, {
    method: 'POST',
    headers: {
      'Authorization': `Token ${INFLUX.token}`,
      'Content-Type': 'application/vnd.flux; charset=utf-8',
      'Accept': 'application/json'
    },
    body: flux
  })

  if (!res.ok) {
    const text = await res.text()
    throw new Error(`Influx query failed: ${res.status} ${res.statusText} - ${text}`)
  }

  // Try to parse JSON response. Influx may return CSV depending on server; handle both.
  const ct = res.headers.get('content-type') || ''
  if (ct.includes('application/json')) {
    const json = await res.json()
    return json
  }

  // fallback to text (CSV)
  const text = await res.text()
  return { csv: text }
}
