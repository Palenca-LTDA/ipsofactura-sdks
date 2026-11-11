"""
Utilidades para verificar firmas de webhooks de Ipsofactura.

Ipsofactura firma cada entrega con HMAC-SHA256 sobre la cadena
``"<timestamp>.<raw_body>"`` usando el secreto configurado en el webhook.

Los headers que llegan en cada petición son:
- ``X-Ipso-Timestamp``: unix timestamp (segundos) del momento del envío
- ``X-Ipso-Signature``: HMAC-SHA256 hex

Uso::

    from ipsofactura import verify_webhook_signature

    @app.route("/webhook", methods=["POST"])
    def webhook():
        raw_body = request.get_data(as_text=True)
        timestamp = request.headers.get("X-Ipso-Timestamp", "")
        signature = request.headers.get("X-Ipso-Signature", "")

        if not verify_webhook_signature(raw_body, timestamp, signature, WEBHOOK_SECRET):
            abort(401)

        evento = request.get_json()
        # procesar evento...
"""

import hashlib
import hmac


def verify_webhook_signature(
    payload: str,
    timestamp: str,
    signature: str,
    secret: str,
) -> bool:
    """Verifica la firma HMAC-SHA256 de un evento de webhook entrante.

    Args:
        payload:    Cuerpo crudo de la petición como string (sin parsear).
        timestamp:  Valor del header ``X-Ipso-Timestamp``.
        signature:  Valor del header ``X-Ipso-Signature``.
        secret:     Secreto del webhook configurado en Ipsofactura.

    Returns:
        ``True`` si la firma es válida, ``False`` en caso contrario.

    Note:
        La comparación usa ``hmac.compare_digest`` para prevenir timing attacks.
    """
    expected = hmac.new(
        secret.encode("utf-8"),
        f"{timestamp}.{payload}".encode("utf-8"),
        hashlib.sha256,
    ).hexdigest()

    # compare_digest realiza comparación en tiempo constante
    return hmac.compare_digest(expected, signature)
