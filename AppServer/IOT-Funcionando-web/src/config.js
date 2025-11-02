// Development InfluxDB configuration (for local dev only)
export const INFLUX = {
  url: 'http://localhost:8086', // InfluxDB HTTP API
  token: 'meu-token-super-secreto', // dev token from docker-compose (NOT for production)
  org: 'ufsm-iot',
  bucket: 'sensores'
}
