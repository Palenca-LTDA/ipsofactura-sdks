<?php

declare(strict_types=1);

namespace Ipsofactura;

class IpsofacturaClient
{
    public const SANDBOX_URL = 'https://sandbox.api.ipsofactura.com';
    public const PROD_URL    = 'https://api.ipsofactura.com';

    public CfdiResource $cfdi;
    public EmpresasResource $empresas;
    public WebhooksResource $webhooks;

    private string $apiKey;
    private string $baseUrl;

    public function __construct(string $apiKey, string $baseUrl = self::PROD_URL)
    {
        $this->apiKey  = $apiKey;
        $this->baseUrl = rtrim($baseUrl, '/');

        $this->cfdi     = new CfdiResource($this);
        $this->empresas = new EmpresasResource($this);
        $this->webhooks = new WebhooksResource($this);
    }

    /**
     * Execute an HTTP request against the Ipsofactura API.
     *
     * @param string       $method  HTTP method (GET, POST, PATCH, DELETE)
     * @param string       $path    Path starting with /
     * @param array|null   $body    JSON body (optional)
     * @param array        $query   Query string parameters
     * @return array       Decoded JSON response
     * @throws IpsofacturaException on HTTP error or cURL failure
     */
    public function request(string $method, string $path, ?array $body = null, array $query = []): array
    {
        $url = $this->baseUrl . $path;

        if (!empty($query)) {
            $url .= '?' . http_build_query($query);
        }

        $ch = curl_init();

        $headers = [
            'x-api: ' . $this->apiKey,
            'Content-Type: application/json',
            'Accept: application/json',
        ];

        curl_setopt_array($ch, [
            CURLOPT_URL            => $url,
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_HTTPHEADER     => $headers,
            CURLOPT_TIMEOUT        => 30,
            CURLOPT_FOLLOWLOCATION => true,
        ]);

        switch (strtoupper($method)) {
            case 'GET':
                curl_setopt($ch, CURLOPT_HTTPGET, true);
                break;

            case 'POST':
                curl_setopt($ch, CURLOPT_POST, true);
                if ($body !== null) {
                    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($body));
                }
                break;

            case 'PATCH':
                curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'PATCH');
                if ($body !== null) {
                    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($body));
                }
                break;

            case 'DELETE':
                curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'DELETE');
                break;

            default:
                curl_setopt($ch, CURLOPT_CUSTOMREQUEST, strtoupper($method));
                if ($body !== null) {
                    curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($body));
                }
        }

        $responseBody = curl_exec($ch);
        $curlError    = curl_error($ch);
        $statusCode   = (int) curl_getinfo($ch, CURLINFO_HTTP_CODE);
        curl_close($ch);

        if ($responseBody === false) {
            throw new IpsofacturaException(0, 'CURL_ERROR', 'cURL error: ' . $curlError);
        }

        $decoded = json_decode((string) $responseBody, true);

        if ($statusCode >= 400) {
            $errCode       = $decoded['code']       ?? 'UNKNOWN_ERROR';
            $errMessage    = $decoded['message']    ?? $responseBody;
            $satCode       = $decoded['sat_code']   ?? null;
            $retryAfter    = isset($decoded['retryAfter']) ? (int) $decoded['retryAfter'] : null;

            throw new IpsofacturaException($statusCode, $errCode, $errMessage, $satCode, $retryAfter);
        }

        return $decoded ?? [];
    }
}
