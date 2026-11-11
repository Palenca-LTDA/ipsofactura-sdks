using System.Security.Cryptography;
using System.Text;

namespace Ipsofactura;

/// <summary>
/// Utility methods for validating incoming webhook events.
/// </summary>
public static class WebhookUtils
{
    /// <summary>
    /// Verifies the HMAC-SHA256 signature of an incoming webhook event.
    ///
    /// The signed message is: <c>timestamp + "." + payload</c>.
    /// The signature is compared using a constant-time equality check to prevent timing attacks.
    /// </summary>
    /// <param name="payload">Raw request body string.</param>
    /// <param name="timestamp">Value of the X-Timestamp header.</param>
    /// <param name="signature">Value of the X-Signature header (hex-encoded).</param>
    /// <param name="secret">The webhook secret configured on the Ipsofactura dashboard.</param>
    /// <returns><c>true</c> if the signature is valid; <c>false</c> otherwise.</returns>
    public static bool VerifySignature(string payload, string timestamp, string signature, string secret)
    {
        var message  = $"{timestamp}.{payload}";
        var keyBytes = Encoding.UTF8.GetBytes(secret);
        var msgBytes = Encoding.UTF8.GetBytes(message);

        using var hmac     = new HMACSHA256(keyBytes);
        var computedBytes   = hmac.ComputeHash(msgBytes);
        var computedHex     = Convert.ToHexString(computedBytes).ToLowerInvariant();

        // Constant-time comparison
        return CryptographicOperations.FixedTimeEquals(
            Encoding.UTF8.GetBytes(computedHex),
            Encoding.UTF8.GetBytes(signature.ToLowerInvariant()));
    }
}
