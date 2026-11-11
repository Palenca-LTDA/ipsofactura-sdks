namespace Ipsofactura;

/// <summary>
/// Exception thrown when the Ipsofactura API returns an HTTP 4xx or 5xx response.
/// </summary>
public sealed class IpsofacturaException : Exception
{
    /// <summary>HTTP status code (e.g. 400, 422, 500).</summary>
    public int StatusCode { get; }

    /// <summary>Machine-readable error code from the API (e.g. "VALIDATION_ERROR").</summary>
    public string Code { get; }

    /// <summary>SAT CFDI error code if applicable (e.g. "CFDI40xxx").</summary>
    public string? SatCode { get; }

    /// <summary>Seconds to wait before retrying, if the API included this hint.</summary>
    public int? RetryAfter { get; }

    public IpsofacturaException(int statusCode, string code, string message, string? satCode = null, int? retryAfter = null)
        : base(message)
    {
        StatusCode = statusCode;
        Code       = code;
        SatCode    = satCode;
        RetryAfter = retryAfter;
    }

    public override string ToString() =>
        $"IpsofacturaException: HTTP {StatusCode} [{Code}] {Message}" +
        (SatCode is not null ? $" (SAT: {SatCode})" : "") +
        (RetryAfter is not null ? $" [retry after {RetryAfter}s]" : "");
}
