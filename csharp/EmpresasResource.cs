namespace Ipsofactura;

/// <summary>
/// Empresa operations: create, update, manage certificados CSD.
/// </summary>
public sealed class EmpresasResource
{
    private readonly IpsofacturaClient _client;

    internal EmpresasResource(IpsofacturaClient client) => _client = client;

    /// <summary>
    /// Lista las empresas. Parámetros: q (búsqueda), include_certificados.
    /// </summary>
    public Task<Dictionary<string, object?>> ListAsync(
        Dictionary<string, string>? query = null, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, "/empresas", query: query, cancellationToken: ct);

    /// <summary>Crea una nueva empresa.</summary>
    public Task<Dictionary<string, object?>> CrearAsync(
        Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, "/empresas", payload, cancellationToken: ct);

    /// <summary>Obtiene una empresa por ID.</summary>
    public Task<Dictionary<string, object?>> GetAsync(string id, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Get, $"/empresas/{Uri.EscapeDataString(id)}", cancellationToken: ct);

    /// <summary>Actualiza los datos de una empresa.</summary>
    public Task<Dictionary<string, object?>> ActualizarAsync(
        string id, Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(new HttpMethod("PATCH"), $"/empresas/{Uri.EscapeDataString(id)}", payload, cancellationToken: ct);

    /// <summary>Sube un certificado CSD a una empresa.</summary>
    public Task<Dictionary<string, object?>> SubirCertificadoAsync(
        string empresaId, Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, $"/empresas/{Uri.EscapeDataString(empresaId)}/certificados", payload, cancellationToken: ct);

    /// <summary>Refresca la contraseña del certificado de una empresa.</summary>
    public Task<Dictionary<string, object?>> RefreshPasswordAsync(
        string empresaId, Dictionary<string, object?> payload, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Post, $"/empresas/{Uri.EscapeDataString(empresaId)}/certificados/refresh-password", payload, cancellationToken: ct);

    /// <summary>Elimina un certificado por número de serie.</summary>
    public Task<Dictionary<string, object?>> EliminarCertificadoAsync(
        string empresaId, string numero, CancellationToken ct = default)
        => _client.RequestAsync(HttpMethod.Delete, $"/empresas/{Uri.EscapeDataString(empresaId)}/certificados/{Uri.EscapeDataString(numero)}", cancellationToken: ct);
}
