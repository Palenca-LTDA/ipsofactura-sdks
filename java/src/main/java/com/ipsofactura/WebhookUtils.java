package com.ipsofactura;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.InvalidKeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

/**
 * Utilidades para verificar firmas de webhooks de Ipsofactura.
 *
 * <p>Ipsofactura firma cada evento con HMAC-SHA256 usando el secreto del webhook.
 * El mensaje firmado tiene la forma: {@code "<timestamp>.<payload>"}.
 *
 * <pre>{@code
 * boolean ok = WebhookUtils.verifySignature(
 *     requestBody,         // body crudo del POST
 *     request.getHeader("x-ipso-timestamp"),
 *     request.getHeader("x-ipso-signature"),
 *     webhookSecret
 * );
 * if (!ok) throw new SecurityException("Firma de webhook inválida");
 * }</pre>
 */
public final class WebhookUtils {

    private static final String HMAC_ALGORITHM = "HmacSHA256";

    private WebhookUtils() {
        // utility class — no instances
    }

    /**
     * Verifica que la firma de un evento webhook sea auténtica.
     *
     * @param payload   Cuerpo crudo del request HTTP (string, no base64)
     * @param timestamp Valor del header {@code x-ipso-timestamp} enviado por Ipsofactura
     * @param signature Valor del header {@code x-ipso-signature} enviado por Ipsofactura
     * @param secret    Secreto del webhook obtenido al crear el webhook en Ipsofactura
     * @return {@code true} si la firma es válida, {@code false} en caso contrario
     */
    public static boolean verifySignature(String payload, String timestamp, String signature, String secret) {
        if (payload == null || timestamp == null || signature == null || secret == null) {
            return false;
        }

        try {
            String message  = timestamp + "." + payload;
            String expected = hmacSha256Hex(secret, message);
            return constantTimeEquals(expected, signature);
        } catch (Exception e) {
            return false;
        }
    }

    /**
     * Computes HMAC-SHA256 and returns the result as a lowercase hex string.
     */
    private static String hmacSha256Hex(String secret, String message)
            throws NoSuchAlgorithmException, InvalidKeyException {

        Mac mac = Mac.getInstance(HMAC_ALGORITHM);
        SecretKeySpec keySpec = new SecretKeySpec(
                secret.getBytes(StandardCharsets.UTF_8),
                HMAC_ALGORITHM
        );
        mac.init(keySpec);
        byte[] raw = mac.doFinal(message.getBytes(StandardCharsets.UTF_8));
        return bytesToHex(raw);
    }

    /**
     * Constant-time string comparison to prevent timing attacks.
     * Uses {@link MessageDigest#isEqual} on the byte representations.
     */
    private static boolean constantTimeEquals(String a, String b) {
        return MessageDigest.isEqual(
                a.getBytes(StandardCharsets.UTF_8),
                b.getBytes(StandardCharsets.UTF_8)
        );
    }

    private static String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder(bytes.length * 2);
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }
}
