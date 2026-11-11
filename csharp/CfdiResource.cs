namespace Ipsofactura;

/// <summary>
/// CFDI operations: timbrar, cancelar, consultar, descargar documentos.
/// </summary>
public sealed class CfdiResource
{
    private readonly IpsofacturaClient _client;

    internal CfdiResource(IpsofacturaClient client) => _client = client;

    /// <summary>Timbra un CFDI 4.0.</summary>
    public Task<Dictionary<string, object?>> TimbrarAsync(
        Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, "/cfdi/timbrar", payload, cancellationToken: ct);

    /// <summary>Timbra un CFDI de nómina.</summary>
    public Task<Dictionary<string, object?>> TimbrarNominaAsync(
        Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, "/cfdi/timbrar-nomina", payload, cancellationToken: ct);

    /// <summary>Timbra desde un XML ya generado.</summary>
    public Task<Dictionary<string, object?>> TimbrarXmlAsync(
        Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, "/cfdi/timbrar-xml", payload, cancellationToken: ct);

    /// <summary>Timbra un comprobante pre-sellado.</summary>
    public Task<Dictionary<string, object?>> TimbrarSelladoAsync(
        Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, "/cfdi/timbrar-sellado", payload, cancellationToken: ct);

    /// <summary>Cancela un CFDI timbrado.</summary>
    public Task<Dictionary<string, object?>> CancelarAsync(
        Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, "/cfdi/cancelar", payload, cancellationToken: ct);

    /// <summary>Agrega un complemento de pago.</summary>
    public Task<Dictionary<string, object?>> ComplementoPagoAsync(
        Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, "/cfdi/complemento-pago", payload, cancellationToken: ct);

    /// <summary>Crea un job de exportación masiva de CFDIs.</summary>
    public Task<Dictionary<string, object?>> CrearExportAsync(
        Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, "/cfdi/exports", payload, cancellationToken: ct);

    /// <summary>Consulta el estado de un export por ID.</summary>
    public Task<Dictionary<string, object?>> GetExportAsync(string id, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, $"/cfdi/exports/{Uri.EscapeDataString(id)}", cancellationToken: ct);

    /// <summary>
    /// Lista CFDIs con filtros opcionales.
    /// Parámetros admitidos: page, per_page, tipo_comprobante, serie, folio, rfc_emisor, folio_fiscal.
    /// </summary>
    public Task<Dictionary<string, object?>> ListAsync(
        Dictionary<string, string>? query = null, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, "/cfdi", query: query, cancellationToken: ct);

    /// <summary>Obtiene un CFDI por ID.</summary>
    public Task<Dictionary<string, object?>> GetAsync(string id, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, $"/cfdi/{Uri.EscapeDataString(id)}", cancellationToken: ct);

    /// <summary>Consulta el estatus SAT de un CFDI.</summary>
    public Task<Dictionary<string, object?>> GetEstatusAsync(string id, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, $"/cfdi/{Uri.EscapeDataString(id)}/estatus", cancellationToken: ct);

    /// <summary>Descarga el XML de un CFDI.</summary>
    public Task<Dictionary<string, object?>> GetXmlAsync(string id, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, $"/cfdi/{Uri.EscapeDataString(id)}/xml", cancellationToken: ct);

    /// <summary>Descarga el PDF de un CFDI.</summary>
    public Task<Dictionary<string, object?>> GetPdfAsync(string id, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, $"/cfdi/{Uri.EscapeDataString(id)}/pdf", cancellationToken: ct);

    /// <summary>Obtiene el acuse de cancelación de un CFDI.</summary>
    public Task<Dictionary<string, object?>> GetAcuseAsync(string id, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, $"/cfdi/{Uri.EscapeDataString(id)}/acuse", cancellationToken: ct);
}
