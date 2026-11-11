"""
Clientes síncrono y asíncrono para la API de Ipsofactura.

Uso síncrono::

    from ipsofactura import IpsofacturaClient

    with IpsofacturaClient("tu-api-key") as client:
        cfdi = client.cfdi.timbrar({...})
        print(cfdi["folio_fiscal"])

Uso asíncrono::

    from ipsofactura import AsyncIpsofacturaClient

    async with AsyncIpsofacturaClient("tu-api-key") as client:
        cfdi = await client.cfdi.timbrar({...})
        print(cfdi["folio_fiscal"])
"""

from __future__ import annotations

from typing import Any, Optional

import httpx

# ---------------------------------------------------------------------------
# Excepción
# ---------------------------------------------------------------------------

SANDBOX_BASE_URL = "https://sandbox.api.ipsofactura.com"
PRODUCTION_BASE_URL = "https://api.ipsofactura.com"
DEFAULT_TIMEOUT = 30.0
SDK_VERSION = "0.1.0"


class IpsofacturaError(Exception):
    """Error estructurado devuelto por la API de Ipsofactura.

    Attributes:
        status_code: Código de estado HTTP (0 para errores de red).
        code:        Código de error de la API (p.ej. ``"VALIDATION_ERROR"``).
        message:     Descripción legible del error.
        sat_code:    Código de error del SAT cuando aplica (p.ej. ``"CFDI40xxx"``).
        retry_after: Segundos que hay que esperar antes de reintentar (si lo indica la API).
    """

    def __init__(
        self,
        status_code: int,
        code: str,
        message: str,
        sat_code: Optional[str] = None,
        retry_after: Optional[int] = None,
    ) -> None:
        super().__init__(message)
        self.status_code = status_code
        self.code = code
        self.message = message
        self.sat_code = sat_code
        self.retry_after = retry_after

    def __repr__(self) -> str:
        parts = [f"status_code={self.status_code}", f"code={self.code!r}"]
        if self.sat_code:
            parts.append(f"sat_code={self.sat_code!r}")
        if self.retry_after is not None:
            parts.append(f"retry_after={self.retry_after}")
        return f"IpsofacturaError({', '.join(parts)})"


# ---------------------------------------------------------------------------
# Helpers internos
# ---------------------------------------------------------------------------

def _build_headers(api_key: str) -> dict[str, str]:
    return {
        "x-api": api_key,
        "Content-Type": "application/json",
        "Accept": "application/json",
        "User-Agent": f"ipsofactura-sdk-python/{SDK_VERSION}",
    }


def _filter_params(params: dict[str, Any]) -> dict[str, str]:
    """Elimina valores None/vacíos y convierte todo a str para query params."""
    return {k: str(v) for k, v in params.items() if v is not None}


def _raise_for_error(response: httpx.Response) -> None:
    """Lanza IpsofacturaError si el status code indica un error."""
    if response.is_success:
        return

    try:
        data = response.json()
    except Exception:
        data = {}

    if isinstance(data, dict):
        raise IpsofacturaError(
            status_code=response.status_code,
            code=str(data.get("code", "UNKNOWN_ERROR")),
            message=str(data.get("message", response.reason_phrase or "HTTP error")),
            sat_code=str(data["sat_code"]) if data.get("sat_code") else None,
            retry_after=int(data["retryAfter"]) if data.get("retryAfter") else None,
        )

    raise IpsofacturaError(
        status_code=response.status_code,
        code="HTTP_ERROR",
        message=f"HTTP {response.status_code}: {response.reason_phrase}",
    )


# ---------------------------------------------------------------------------
# Recursos síncronos
# ---------------------------------------------------------------------------

class _CfdiResource:
    """Operaciones sobre CFDIs (timbrado, cancelación, consulta)."""

    def __init__(self, client: "IpsofacturaClient") -> None:
        self._client = client

    def timbrar(self, payload: dict[str, Any]) -> Any:
        """Timbra una factura CFDI 4.0 (tipo I, E, T, N, P)."""
        return self._client._post("/cfdi/timbrar", payload)

    def timbrar_nomina(self, payload: dict[str, Any]) -> Any:
        """Timbra un CFDI de Nómina (tipo N, complemento nómina 1.2)."""
        return self._client._post("/cfdi/timbrar-nomina", payload)

    def timbrar_xml(self, payload: dict[str, Any]) -> Any:
        """Timbra un XML ya generado externamente."""
        return self._client._post("/cfdi/timbrar-xml", payload)

    def timbrar_sellado(self, payload: dict[str, Any]) -> Any:
        """Timbra un XML ya sellado (con cadena original y sello del emisor)."""
        return self._client._post("/cfdi/timbrar-sellado", payload)

    def cancelar(self, payload: dict[str, Any]) -> Any:
        """Solicita la cancelación de un CFDI ante el SAT."""
        return self._client._post("/cfdi/cancelar", payload)

    def complemento_pago(self, payload: dict[str, Any]) -> Any:
        """Genera un complemento de pago (tipo P) para una factura con PPD."""
        return self._client._post("/cfdi/complemento-pago", payload)

    def crear_export(self, payload: dict[str, Any]) -> Any:
        """Crea un trabajo de exportación masiva de CFDIs."""
        return self._client._post("/cfdi/exports", payload)

    def get_export(self, export_id: str) -> Any:
        """Consulta el estado de un export. Cuando status='ready', incluye download_url."""
        return self._client._get(f"/cfdi/exports/{export_id}")

    def list(
        self,
        page: Optional[int] = None,
        per_page: Optional[int] = None,
        tipo_comprobante: Optional[str] = None,
        serie: Optional[str] = None,
        folio: Optional[str] = None,
        rfc_emisor: Optional[str] = None,
        folio_fiscal: Optional[str] = None,
    ) -> Any:
        """Lista CFDIs con paginación y filtros opcionales."""
        return self._client._get(
            "/cfdi",
            params=_filter_params({
                "page": page,
                "per_page": per_page,
                "tipo_comprobante": tipo_comprobante,
                "serie": serie,
                "folio": folio,
                "rfc_emisor": rfc_emisor,
                "folio_fiscal": folio_fiscal,
            }),
        )

    def get(self, cfdi_id: str) -> Any:
        """Obtiene un CFDI por su ID interno de Ipsofactura."""
        return self._client._get(f"/cfdi/{cfdi_id}")

    def get_estatus(self, cfdi_id: str) -> Any:
        """Consulta el estatus del CFDI ante el SAT (vigente, cancelado, etc.)."""
        return self._client._get(f"/cfdi/{cfdi_id}/estatus")

    def get_xml(self, cfdi_id: str) -> Any:
        """Obtiene la URL firmada del XML del CFDI. Retorna ``{"xml_url": str}``."""
        return self._client._get(f"/cfdi/{cfdi_id}/xml")

    def get_pdf(self, cfdi_id: str) -> Any:
        """Obtiene la URL firmada del PDF. Retorna ``{"pdf_url": str}``."""
        return self._client._get(f"/cfdi/{cfdi_id}/pdf")

    def get_acuse(self, cfdi_id: str) -> Any:
        """Obtiene el acuse de cancelación de un CFDI cancelado."""
        return self._client._get(f"/cfdi/{cfdi_id}/acuse")


class _EmpresasResource:
    """Gestión de empresas emisoras y sus certificados CSD."""

    def __init__(self, client: "IpsofacturaClient") -> None:
        self._client = client

    def list(
        self,
        q: Optional[str] = None,
        include_certificados: Optional[bool] = None,
    ) -> Any:
        """Lista las empresas registradas. Usa ``q`` para buscar por RFC o nombre."""
        return self._client._get(
            "/empresas",
            params=_filter_params({
                "q": q,
                "include_certificados": include_certificados,
            }),
        )

    def crear(self, payload: dict[str, Any]) -> Any:
        """Registra una nueva empresa emisora."""
        return self._client._post("/empresas", payload)

    def get(self, empresa_id: str) -> Any:
        """Obtiene el detalle de una empresa por su ID."""
        return self._client._get(f"/empresas/{empresa_id}")

    def actualizar(self, empresa_id: str, payload: dict[str, Any]) -> Any:
        """Actualiza campos de una empresa existente (solo envía los campos a modificar)."""
        return self._client._patch(f"/empresas/{empresa_id}", payload)

    def subir_certificado(self, empresa_id: str, payload: dict[str, Any]) -> Any:
        """Sube un certificado CSD (.cer + .key + contraseña) a una empresa.

        El payload debe contener: ``{"cer": "<base64>", "key": "<base64>", "password": "..."}``
        """
        return self._client._post(f"/empresas/{empresa_id}/certificados", payload)

    def refresh_password(self, empresa_id: str, payload: dict[str, Any]) -> Any:
        """Actualiza la contraseña del certificado CSD sin resubirlo."""
        return self._client._post(
            f"/empresas/{empresa_id}/certificados/refresh-password", payload
        )

    def eliminar_certificado(self, empresa_id: str, numero: str) -> Any:
        """Elimina un certificado CSD por su número de serie (20 dígitos)."""
        return self._client._delete(f"/empresas/{empresa_id}/certificados/{numero}")


class _WebhooksResource:
    """Gestión de webhooks y su historial de entregas."""

    def __init__(self, client: "IpsofacturaClient") -> None:
        self._client = client

    def list(self) -> Any:
        """Lista todos los webhooks configurados en la cuenta."""
        return self._client._get("/webhooks")

    def crear(self, payload: dict[str, Any]) -> Any:
        """Crea un nuevo webhook. El payload debe incluir ``url``, ``events`` y opcionalmente ``secret``."""
        return self._client._post("/webhooks", payload)

    def actualizar(self, webhook_id: str, payload: dict[str, Any]) -> Any:
        """Actualiza un webhook existente (URL, eventos, habilitado/deshabilitado)."""
        return self._client._patch(f"/webhooks/{webhook_id}", payload)

    def eliminar(self, webhook_id: str) -> Any:
        """Elimina un webhook permanentemente."""
        return self._client._delete(f"/webhooks/{webhook_id}")

    def list_deliveries(self, webhook_id: str) -> Any:
        """Lista el historial de entregas de un webhook."""
        return self._client._get(f"/webhooks/{webhook_id}/deliveries")

    def get_delivery(self, webhook_id: str, delivery_id: str) -> Any:
        """Obtiene el detalle de una entrega (request, response, status code)."""
        return self._client._get(f"/webhooks/{webhook_id}/deliveries/{delivery_id}")

    def resend_delivery(self, webhook_id: str, delivery_id: str) -> Any:
        """Reintenta una entrega fallida."""
        return self._client._post(
            f"/webhooks/{webhook_id}/deliveries/{delivery_id}/resend", {}
        )

    def test(self, webhook_id: str) -> Any:
        """Envía un payload de prueba al webhook para verificar que funciona."""
        return self._client._post(f"/webhooks/{webhook_id}/test", {})


# ---------------------------------------------------------------------------
# Cliente síncrono
# ---------------------------------------------------------------------------

class IpsofacturaClient:
    """Cliente síncrono de Ipsofactura.

    Soporta uso como context manager::

        with IpsofacturaClient("tu-api-key") as client:
            empresas = client.empresas.list()
            cfdi = client.cfdi.timbrar({...})

    Args:
        api_key:  Tu API key de Ipsofactura.
        base_url: URL base de la API. Por defecto apunta a producción.
                  Para sandbox usa ``"https://sandbox.api.ipsofactura.com"``.
        timeout:  Timeout en segundos para las peticiones HTTP (default: 30).
    """

    def __init__(
        self,
        api_key: str,
        *,
        base_url: str = PRODUCTION_BASE_URL,
        timeout: float = DEFAULT_TIMEOUT,
    ) -> None:
        if not api_key or not isinstance(api_key, str):
            raise ValueError("api_key debe ser un string no vacío")

        self._api_key = api_key
        self._base_url = base_url.rstrip("/")
        self._http = httpx.Client(
            base_url=self._base_url,
            headers=_build_headers(api_key),
            timeout=timeout,
        )

        self._cfdi = _CfdiResource(self)
        self._empresas = _EmpresasResource(self)
        self._webhooks = _WebhooksResource(self)

    @property
    def cfdi(self) -> _CfdiResource:
        """Recurso para operaciones sobre CFDIs."""
        return self._cfdi

    @property
    def empresas(self) -> _EmpresasResource:
        """Recurso para gestión de empresas emisoras."""
        return self._empresas

    @property
    def webhooks(self) -> _WebhooksResource:
        """Recurso para gestión de webhooks."""
        return self._webhooks

    # -------------------------------------------------------------------------
    # Métodos HTTP internos
    # -------------------------------------------------------------------------

    def _get(self, path: str, params: Optional[dict[str, str]] = None) -> Any:
        try:
            response = self._http.get(path, params=params)
        except httpx.RequestError as exc:
            raise IpsofacturaError(
                0, "NETWORK_ERROR", f"Error de red: {exc}"
            ) from exc
        _raise_for_error(response)
        if response.status_code == 204:
            return None
        return response.json()

    def _post(self, path: str, payload: dict[str, Any]) -> Any:
        try:
            response = self._http.post(path, json=payload)
        except httpx.RequestError as exc:
            raise IpsofacturaError(
                0, "NETWORK_ERROR", f"Error de red: {exc}"
            ) from exc
        _raise_for_error(response)
        if response.status_code == 204:
            return None
        return response.json()

    def _patch(self, path: str, payload: dict[str, Any]) -> Any:
        try:
            response = self._http.patch(path, json=payload)
        except httpx.RequestError as exc:
            raise IpsofacturaError(
                0, "NETWORK_ERROR", f"Error de red: {exc}"
            ) from exc
        _raise_for_error(response)
        if response.status_code == 204:
            return None
        return response.json()

    def _delete(self, path: str) -> Any:
        try:
            response = self._http.delete(path)
        except httpx.RequestError as exc:
            raise IpsofacturaError(
                0, "NETWORK_ERROR", f"Error de red: {exc}"
            ) from exc
        _raise_for_error(response)
        if response.status_code == 204:
            return None
        return response.json()

    # -------------------------------------------------------------------------
    # Context manager
    # -------------------------------------------------------------------------

    def close(self) -> None:
        """Cierra el cliente HTTP liberando conexiones."""
        self._http.close()

    def __enter__(self) -> "IpsofacturaClient":
        return self

    def __exit__(self, *args: Any) -> None:
        self.close()


# ---------------------------------------------------------------------------
# Recursos asíncronos
# ---------------------------------------------------------------------------

class _AsyncCfdiResource:
    """Versión async del recurso CFDI."""

    def __init__(self, client: "AsyncIpsofacturaClient") -> None:
        self._client = client

    async def timbrar(self, payload: dict[str, Any]) -> Any:
        """Timbra una factura CFDI 4.0 (tipo I, E, T, N, P)."""
        return await self._client._post("/cfdi/timbrar", payload)

    async def timbrar_nomina(self, payload: dict[str, Any]) -> Any:
        """Timbra un CFDI de Nómina (tipo N, complemento nómina 1.2)."""
        return await self._client._post("/cfdi/timbrar-nomina", payload)

    async def timbrar_xml(self, payload: dict[str, Any]) -> Any:
        """Timbra un XML ya generado externamente."""
        return await self._client._post("/cfdi/timbrar-xml", payload)

    async def timbrar_sellado(self, payload: dict[str, Any]) -> Any:
        """Timbra un XML ya sellado."""
        return await self._client._post("/cfdi/timbrar-sellado", payload)

    async def cancelar(self, payload: dict[str, Any]) -> Any:
        """Solicita la cancelación de un CFDI ante el SAT."""
        return await self._client._post("/cfdi/cancelar", payload)

    async def complemento_pago(self, payload: dict[str, Any]) -> Any:
        """Genera un complemento de pago (tipo P) para una factura con PPD."""
        return await self._client._post("/cfdi/complemento-pago", payload)

    async def crear_export(self, payload: dict[str, Any]) -> Any:
        """Crea un trabajo de exportación masiva de CFDIs."""
        return await self._client._post("/cfdi/exports", payload)

    async def get_export(self, export_id: str) -> Any:
        """Consulta el estado de un export."""
        return await self._client._get(f"/cfdi/exports/{export_id}")

    async def list(
        self,
        page: Optional[int] = None,
        per_page: Optional[int] = None,
        tipo_comprobante: Optional[str] = None,
        serie: Optional[str] = None,
        folio: Optional[str] = None,
        rfc_emisor: Optional[str] = None,
        folio_fiscal: Optional[str] = None,
    ) -> Any:
        """Lista CFDIs con paginación y filtros opcionales."""
        return await self._client._get(
            "/cfdi",
            params=_filter_params({
                "page": page,
                "per_page": per_page,
                "tipo_comprobante": tipo_comprobante,
                "serie": serie,
                "folio": folio,
                "rfc_emisor": rfc_emisor,
                "folio_fiscal": folio_fiscal,
            }),
        )

    async def get(self, cfdi_id: str) -> Any:
        """Obtiene un CFDI por su ID interno."""
        return await self._client._get(f"/cfdi/{cfdi_id}")

    async def get_estatus(self, cfdi_id: str) -> Any:
        """Consulta el estatus del CFDI ante el SAT."""
        return await self._client._get(f"/cfdi/{cfdi_id}/estatus")

    async def get_xml(self, cfdi_id: str) -> Any:
        """Obtiene la URL firmada del XML del CFDI."""
        return await self._client._get(f"/cfdi/{cfdi_id}/xml")

    async def get_pdf(self, cfdi_id: str) -> Any:
        """Obtiene la URL firmada del PDF."""
        return await self._client._get(f"/cfdi/{cfdi_id}/pdf")

    async def get_acuse(self, cfdi_id: str) -> Any:
        """Obtiene el acuse de cancelación de un CFDI cancelado."""
        return await self._client._get(f"/cfdi/{cfdi_id}/acuse")


class _AsyncEmpresasResource:
    """Versión async del recurso Empresas."""

    def __init__(self, client: "AsyncIpsofacturaClient") -> None:
        self._client = client

    async def list(
        self,
        q: Optional[str] = None,
        include_certificados: Optional[bool] = None,
    ) -> Any:
        """Lista las empresas registradas."""
        return await self._client._get(
            "/empresas",
            params=_filter_params({
                "q": q,
                "include_certificados": include_certificados,
            }),
        )

    async def crear(self, payload: dict[str, Any]) -> Any:
        """Registra una nueva empresa emisora."""
        return await self._client._post("/empresas", payload)

    async def get(self, empresa_id: str) -> Any:
        """Obtiene el detalle de una empresa por su ID."""
        return await self._client._get(f"/empresas/{empresa_id}")

    async def actualizar(self, empresa_id: str, payload: dict[str, Any]) -> Any:
        """Actualiza campos de una empresa existente."""
        return await self._client._patch(f"/empresas/{empresa_id}", payload)

    async def subir_certificado(self, empresa_id: str, payload: dict[str, Any]) -> Any:
        """Sube un certificado CSD a una empresa."""
        return await self._client._post(
            f"/empresas/{empresa_id}/certificados", payload
        )

    async def refresh_password(self, empresa_id: str, payload: dict[str, Any]) -> Any:
        """Actualiza la contraseña del certificado CSD sin resubirlo."""
        return await self._client._post(
            f"/empresas/{empresa_id}/certificados/refresh-password", payload
        )

    async def eliminar_certificado(self, empresa_id: str, numero: str) -> Any:
        """Elimina un certificado CSD por su número de serie."""
        return await self._client._delete(
            f"/empresas/{empresa_id}/certificados/{numero}"
        )


class _AsyncWebhooksResource:
    """Versión async del recurso Webhooks."""

    def __init__(self, client: "AsyncIpsofacturaClient") -> None:
        self._client = client

    async def list(self) -> Any:
        """Lista todos los webhooks."""
        return await self._client._get("/webhooks")

    async def crear(self, payload: dict[str, Any]) -> Any:
        """Crea un nuevo webhook."""
        return await self._client._post("/webhooks", payload)

    async def actualizar(self, webhook_id: str, payload: dict[str, Any]) -> Any:
        """Actualiza un webhook existente."""
        return await self._client._patch(f"/webhooks/{webhook_id}", payload)

    async def eliminar(self, webhook_id: str) -> Any:
        """Elimina un webhook permanentemente."""
        return await self._client._delete(f"/webhooks/{webhook_id}")

    async def list_deliveries(self, webhook_id: str) -> Any:
        """Lista el historial de entregas de un webhook."""
        return await self._client._get(f"/webhooks/{webhook_id}/deliveries")

    async def get_delivery(self, webhook_id: str, delivery_id: str) -> Any:
        """Obtiene el detalle de una entrega específica."""
        return await self._client._get(
            f"/webhooks/{webhook_id}/deliveries/{delivery_id}"
        )

    async def resend_delivery(self, webhook_id: str, delivery_id: str) -> Any:
        """Reintenta una entrega fallida."""
        return await self._client._post(
            f"/webhooks/{webhook_id}/deliveries/{delivery_id}/resend", {}
        )

    async def test(self, webhook_id: str) -> Any:
        """Envía un payload de prueba al webhook."""
        return await self._client._post(f"/webhooks/{webhook_id}/test", {})


# ---------------------------------------------------------------------------
# Cliente asíncrono
# ---------------------------------------------------------------------------

class AsyncIpsofacturaClient:
    """Cliente asíncrono de Ipsofactura (usa ``httpx.AsyncClient``).

    Soporta uso como async context manager::

        async with AsyncIpsofacturaClient("tu-api-key") as client:
            empresas = await client.empresas.list()
            cfdi = await client.cfdi.timbrar({...})

    Args:
        api_key:  Tu API key de Ipsofactura.
        base_url: URL base de la API. Por defecto apunta a producción.
                  Para sandbox usa ``"https://sandbox.api.ipsofactura.com"``.
        timeout:  Timeout en segundos para las peticiones HTTP (default: 30).
    """

    def __init__(
        self,
        api_key: str,
        *,
        base_url: str = PRODUCTION_BASE_URL,
        timeout: float = DEFAULT_TIMEOUT,
    ) -> None:
        if not api_key or not isinstance(api_key, str):
            raise ValueError("api_key debe ser un string no vacío")

        self._api_key = api_key
        self._base_url = base_url.rstrip("/")
        self._http = httpx.AsyncClient(
            base_url=self._base_url,
            headers=_build_headers(api_key),
            timeout=timeout,
        )

        self._cfdi = _AsyncCfdiResource(self)
        self._empresas = _AsyncEmpresasResource(self)
        self._webhooks = _AsyncWebhooksResource(self)

    @property
    def cfdi(self) -> _AsyncCfdiResource:
        """Recurso para operaciones sobre CFDIs."""
        return self._cfdi

    @property
    def empresas(self) -> _AsyncEmpresasResource:
        """Recurso para gestión de empresas emisoras."""
        return self._empresas

    @property
    def webhooks(self) -> _AsyncWebhooksResource:
        """Recurso para gestión de webhooks."""
        return self._webhooks

    # -------------------------------------------------------------------------
    # Métodos HTTP internos
    # -------------------------------------------------------------------------

    async def _get(self, path: str, params: Optional[dict[str, str]] = None) -> Any:
        try:
            response = await self._http.get(path, params=params)
        except httpx.RequestError as exc:
            raise IpsofacturaError(
                0, "NETWORK_ERROR", f"Error de red: {exc}"
            ) from exc
        _raise_for_error(response)
        if response.status_code == 204:
            return None
        return response.json()

    async def _post(self, path: str, payload: dict[str, Any]) -> Any:
        try:
            response = await self._http.post(path, json=payload)
        except httpx.RequestError as exc:
            raise IpsofacturaError(
                0, "NETWORK_ERROR", f"Error de red: {exc}"
            ) from exc
        _raise_for_error(response)
        if response.status_code == 204:
            return None
        return response.json()

    async def _patch(self, path: str, payload: dict[str, Any]) -> Any:
        try:
            response = await self._http.patch(path, json=payload)
        except httpx.RequestError as exc:
            raise IpsofacturaError(
                0, "NETWORK_ERROR", f"Error de red: {exc}"
            ) from exc
        _raise_for_error(response)
        if response.status_code == 204:
            return None
        return response.json()

    async def _delete(self, path: str) -> Any:
        try:
            response = await self._http.delete(path)
        except httpx.RequestError as exc:
            raise IpsofacturaError(
                0, "NETWORK_ERROR", f"Error de red: {exc}"
            ) from exc
        _raise_for_error(response)
        if response.status_code == 204:
            return None
        return response.json()

    # -------------------------------------------------------------------------
    # Context manager
    # -------------------------------------------------------------------------

    async def aclose(self) -> None:
        """Cierra el cliente HTTP asíncrono liberando conexiones."""
        await self._http.aclose()

    async def __aenter__(self) -> "AsyncIpsofacturaClient":
        return self

    async def __aexit__(self, *args: Any) -> None:
        await self.aclose()
