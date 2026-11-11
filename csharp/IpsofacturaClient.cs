using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;

namespace Ipsofactura;

/// <summary>
/// Main client for the Ipsofactura API.
/// </summary>
public sealed class IpsofacturaClient : IDisposable
{
    public const string SandboxUrl = "https://sandbox.api.ipsofactura.com";
    public const string ProdUrl    = "https://api.ipsofactura.com";

    private readonly HttpClient _http;
    private bool _disposed;

    /// <summary>CFDI operations (timbrar, cancelar, consultar, etc.).</summary>
    public CfdiResource Cfdi { get; }

    /// <summary>Empresa management (create, update, certificados).</summary>
    public EmpresasResource Empresas { get; }

    /// <summary>Webhook management and delivery history.</summary>
    public WebhooksResource Webhooks { get; }

    /// <summary>
    /// Creates a new IpsofacturaClient.
    /// </summary>
    /// <param name="apiKey">Your Ipsofactura API key.</param>
    /// <param name="baseUrl">Base URL. Defaults to production. Pass <see cref="SandboxUrl"/> for testing.</param>
    /// <param name="httpClient">Optional existing HttpClient to use (useful for testing).</param>
    public IpsofacturaClient(string apiKey, string? baseUrl = null, HttpClient? httpClient = null)
    {
        var effectiveBaseUrl = (baseUrl ?? ProdUrl).TrimEnd('/');

        _http = httpClient ?? new HttpClient
        {
            BaseAddress = new Uri(effectiveBaseUrl),
            Timeout     = TimeSpan.FromSeconds(30),
        };

        if (_http.BaseAddress is null)
        {
            _http.BaseAddress = new Uri(effectiveBaseUrl);
        }

        _http.DefaultRequestHeaders.Add("x-api", apiKey);
        _http.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));

        Cfdi     = new CfdiResource(this);
        Empresas = new EmpresasResource(this);
        Webhooks = new WebhooksResource(this);
    }

    /// <summary>
    /// Executes an HTTP request and returns the decoded JSON response.
    /// Throws <see cref="IpsofacturaException"/> for HTTP 4xx/5xx responses.
    /// </summary>
    internal async Task<Dictionary<string, object?>> RequestAsync(
        HttpMethod method,
        string path,
        Dictionary<string, object?>? body = null,
        Dictionary<string, string>? query = null,
        CancellationToken cancellationToken = default)
    {
        var url = path;
        if (query is { Count: > 0 })
        {
            var qs = string.Join("&", query.Select(kv => $"{Uri.EscapeDataString(kv.Key)}={Uri.EscapeDataString(kv.Value)}"));
            url = $"{path}?{qs}";
        }

        HttpContent? content = null;
        if (body is not null)
        {
            var json = JsonSerializer.Serialize(body);
            content = new StringContent(json, Encoding.UTF8, "application/json");
        }

        var request = new HttpRequestMessage(method, url) { Content = content };
        var response = await _http.SendAsync(request, cancellationToken).ConfigureAwait(false);

        var responseText = await response.Content.ReadAsStringAsync(cancellationToken).ConfigureAwait(false);

        Dictionary<string, object?>? decoded = null;
        if (!string.IsNullOrWhiteSpace(responseText))
        {
            try
            {
                decoded = JsonSerializer.Deserialize<Dictionary<string, object?>>(responseText);
            }
            catch
            {
                decoded = new Dictionary<string, object?> { ["raw"] = responseText };
            }
        }

        if (!response.IsSuccessStatusCode)
        {
            var statusCode  = (int)response.StatusCode;
            var errCode     = GetString(decoded, "code")     ?? "UNKNOWN_ERROR";
            var errMessage  = GetString(decoded, "message")  ?? responseText;
            var satCode     = GetString(decoded, "sat_code");
            int? retryAfter = null;
            if (decoded?.TryGetValue("retryAfter", out var ra) == true && ra is JsonElement raEl && raEl.ValueKind == JsonValueKind.Number)
            {
                retryAfter = raEl.GetInt32();
            }

            throw new IpsofacturaException(statusCode, errCode, errMessage, satCode, retryAfter);
        }

        return decoded ?? new Dictionary<string, object?>();
    }

    private static string? GetString(Dictionary<string, object?>? dict, string key)
    {
        if (dict is null || !dict.TryGetValue(key, out var val)) return null;
        if (val is JsonElement je && je.ValueKind == JsonValueKind.String) return je.GetString();
        return val?.ToString();
    }

    public void Dispose()
    {
        if (_disposed) return;
        _http.Dispose();
        _disposed = true;
    }
}
