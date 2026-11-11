package com.ipsofactura;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.reflect.TypeToken;

import java.io.IOException;
import java.lang.reflect.Type;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.Map;

/**
 * Main entry point for the Ipsofactura SDK.
 *
 * <pre>{@code
 * // Sandbox
 * IpsofacturaClient client = new IpsofacturaClient("mi_api_key", IpsofacturaClient.SANDBOX_URL);
 *
 * // Producción (por defecto)
 * IpsofacturaClient client = new IpsofacturaClient("mi_api_key");
 * }</pre>
 */
public class IpsofacturaClient {

    public static final String SANDBOX_URL = "https://sandbox.api.ipsofactura.com";
    public static final String PROD_URL    = "https://api.ipsofactura.com";

    private static final Type MAP_TYPE = new TypeToken<Map<String, Object>>() {}.getType();

    private final String apiKey;
    private final String baseUrl;
    private final HttpClient http;
    private final Gson gson;

    // Resource handles — created once and re-used
    private final CfdiResource     cfdiResource;
    private final EmpresasResource empresasResource;
    private final WebhooksResource webhooksResource;

    /** Creates a client pointing at the production environment. */
    public IpsofacturaClient(String apiKey) {
        this(apiKey, PROD_URL);
    }

    /** Creates a client with a custom base URL (useful for sandbox or mocking). */
    public IpsofacturaClient(String apiKey, String baseUrl) {
        if (apiKey == null || apiKey.isBlank()) {
            throw new IllegalArgumentException("apiKey must not be blank");
        }
        this.apiKey   = apiKey;
        this.baseUrl  = baseUrl.endsWith("/") ? baseUrl.substring(0, baseUrl.length() - 1) : baseUrl;
        this.http     = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(10))
                .build();
        this.gson     = new GsonBuilder().create();

        this.cfdiResource     = new CfdiResource(this);
        this.empresasResource = new EmpresasResource(this);
        this.webhooksResource = new WebhooksResource(this);
    }

    /** Access CFDI timbrado, cancelación, exports, etc. */
    public CfdiResource cfdi() {
        return cfdiResource;
    }

    /** Access empresa / certificado management. */
    public EmpresasResource empresas() {
        return empresasResource;
    }

    /** Access webhook configuration and deliveries. */
    public WebhooksResource webhooks() {
        return webhooksResource;
    }

    // -------------------------------------------------------------------------
    // Package-private HTTP helpers used by resource classes
    // -------------------------------------------------------------------------

    /**
     * Executes a request with an optional JSON body.
     *
     * @param method HTTP method (GET, POST, PATCH, DELETE)
     * @param path   path relative to baseUrl (e.g. "/cfdi/timbrar")
     * @param body   object to serialize as JSON body, or {@code null} for bodyless requests
     * @return parsed response as {@code Map<String,Object>}
     */
    Map<String, Object> request(String method, String path, Object body) {
        return request(method, path, body, null);
    }

    /**
     * Executes a GET request with optional query parameters.
     *
     * @param path        path relative to baseUrl
     * @param queryParams query parameters, or {@code null}
     * @return parsed response as {@code Map<String,Object>}
     */
    Map<String, Object> get(String path, Map<String, String> queryParams) {
        return request("GET", path, null, queryParams);
    }

    /**
     * Full internal request dispatcher.
     */
    Map<String, Object> request(String method, String path, Object body, Map<String, String> queryParams) {
        try {
            String url = baseUrl + path;
            if (queryParams != null && !queryParams.isEmpty()) {
                StringBuilder sb = new StringBuilder(url).append('?');
                boolean first = true;
                for (Map.Entry<String, String> entry : queryParams.entrySet()) {
                    if (!first) sb.append('&');
                    sb.append(encodeURIComponent(entry.getKey()))
                      .append('=')
                      .append(encodeURIComponent(entry.getValue()));
                    first = false;
                }
                url = sb.toString();
            }

            HttpRequest.Builder builder = HttpRequest.newBuilder()
                    .uri(URI.create(url))
                    .header("x-api", apiKey)
                    .header("Content-Type", "application/json")
                    .header("Accept", "application/json");

            if (body != null) {
                String json = gson.toJson(body);
                builder.method(method, HttpRequest.BodyPublishers.ofString(json));
            } else if ("DELETE".equals(method) || "GET".equals(method)) {
                builder.method(method, HttpRequest.BodyPublishers.noBody());
            } else {
                // POST/PATCH with no body — send empty JSON object
                builder.method(method, HttpRequest.BodyPublishers.ofString("{}"));
            }

            HttpResponse<String> response = http.send(builder.build(), HttpResponse.BodyHandlers.ofString());
            return handleResponse(response);

        } catch (IpsofacturaException e) {
            throw e;
        } catch (IOException | InterruptedException e) {
            throw new RuntimeException("HTTP request failed: " + e.getMessage(), e);
        }
    }

    private Map<String, Object> handleResponse(HttpResponse<String> response) {
        int status = response.statusCode();
        String responseBody = response.body();

        if (status >= 200 && status < 300) {
            if (responseBody == null || responseBody.isBlank()) {
                return Map.of();
            }
            Map<String, Object> parsed = gson.fromJson(responseBody, MAP_TYPE);
            return parsed != null ? parsed : Map.of();
        }

        // Parse error body
        String code       = "UNKNOWN_ERROR";
        String message    = "HTTP " + status;
        String satCode    = null;
        Integer retryAfter = null;

        if (responseBody != null && !responseBody.isBlank()) {
            try {
                Map<String, Object> err = gson.fromJson(responseBody, MAP_TYPE);
                if (err.containsKey("code"))       code       = (String) err.get("code");
                if (err.containsKey("message"))    message    = (String) err.get("message");
                if (err.containsKey("sat_code"))   satCode    = (String) err.get("sat_code");
                if (err.containsKey("retryAfter")) {
                    Object ra = err.get("retryAfter");
                    if (ra instanceof Number) retryAfter = ((Number) ra).intValue();
                }
            } catch (Exception ignored) {
                message = responseBody;
            }
        }

        throw new IpsofacturaException(status, code, message, satCode, retryAfter);
    }

    private static String encodeURIComponent(String value) {
        try {
            return java.net.URLEncoder.encode(value, "UTF-8")
                    .replace("+", "%20")
                    .replace("%21", "!")
                    .replace("%27", "'")
                    .replace("%28", "(")
                    .replace("%29", ")")
                    .replace("%7E", "~");
        } catch (java.io.UnsupportedEncodingException e) {
            return value;
        }
    }
}
