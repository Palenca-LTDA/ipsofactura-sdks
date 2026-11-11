<?php

declare(strict_types=1);

namespace Ipsofactura;

class WebhookUtils
{
    /**
     * Verifica la firma HMAC-SHA256 de un evento de webhook.
     *
     * @param string $payload    Cuerpo crudo del request (string de bytes)
     * @param string $timestamp  Timestamp enviado en el header (e.g. X-Timestamp)
     * @param string $signature  Firma enviada en el header (e.g. X-Signature), en hex
     * @param string $secret     Secreto compartido del webhook
     * @return bool  true si la firma es válida
     */
    public static function verifySignature(
        string $payload,
        string $timestamp,
        string $signature,
        string $secret
    ): bool {
        // El mensaje firmado es: timestamp + "." + payload
        $message  = $timestamp . '.' . $payload;
        $expected = hash_hmac('sha256', $message, $secret);

        return hash_equals($expected, strtolower($signature));
    }
}
