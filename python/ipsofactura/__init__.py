"""
ipsofactura — Cliente oficial de Ipsofactura para Python

Timbrado CFDI 4.0 para México. Soporta clientes síncronos y asíncronos.

Uso básico::

    from ipsofactura import IpsofacturaClient

    with IpsofacturaClient("tu-api-key") as client:
        cfdi = client.cfdi.timbrar({...})
        print(cfdi["folio_fiscal"])

Uso asíncrono::

    from ipsofactura import AsyncIpsofacturaClient

    async with AsyncIpsofacturaClient("tu-api-key") as client:
        cfdi = await client.cfdi.timbrar({...})

Sandbox::

    from ipsofactura import IpsofacturaClient, SANDBOX_BASE_URL

    client = IpsofacturaClient("tu-api-key", base_url=SANDBOX_BASE_URL)
"""

from ipsofactura.client import (
    AsyncIpsofacturaClient,
    IpsofacturaClient,
    IpsofacturaError,
    PRODUCTION_BASE_URL,
    SANDBOX_BASE_URL,
)
from ipsofactura._webhook_utils import verify_webhook_signature

__all__ = [
    "IpsofacturaClient",
    "AsyncIpsofacturaClient",
    "IpsofacturaError",
    "verify_webhook_signature",
    "PRODUCTION_BASE_URL",
    "SANDBOX_BASE_URL",
]

__version__ = "0.1.0"
