package ipsofactura

import (
	"context"
	"net/url"
)

// CfdiResource groups all CFDI-related API calls.
type CfdiResource struct {
	client *Client
}

// Timbrar stamps a CFDI 4.0 invoice.
func (r *CfdiResource) Timbrar(ctx context.Context, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/cfdi/timbrar", payload, nil)
}

// TimbrarNomina stamps a payroll (nómina) CFDI.
func (r *CfdiResource) TimbrarNomina(ctx context.Context, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/cfdi/timbrar-nomina", payload, nil)
}

// TimbrarXml stamps a CFDI from a pre-generated XML string.
func (r *CfdiResource) TimbrarXml(ctx context.Context, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/cfdi/timbrar-xml", payload, nil)
}

// TimbrarSellado stamps a pre-signed (sellado) CFDI.
func (r *CfdiResource) TimbrarSellado(ctx context.Context, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/cfdi/timbrar-sellado", payload, nil)
}

// Cancelar cancels a stamped CFDI.
func (r *CfdiResource) Cancelar(ctx context.Context, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/cfdi/cancelar", payload, nil)
}

// ComplementoPago adds a payment complement to a CFDI.
func (r *CfdiResource) ComplementoPago(ctx context.Context, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/cfdi/complemento-pago", payload, nil)
}

// CrearExport creates a bulk CFDI export job.
func (r *CfdiResource) CrearExport(ctx context.Context, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/cfdi/exports", payload, nil)
}

// GetExport retrieves the status of a bulk export job.
func (r *CfdiResource) GetExport(ctx context.Context, id string) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/cfdi/exports/"+url.PathEscape(id), nil, nil)
}

// List returns a paginated list of CFDIs.
// Supported params: page, per_page, tipo_comprobante, serie, folio, rfc_emisor, folio_fiscal.
func (r *CfdiResource) List(ctx context.Context, params url.Values) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/cfdi", nil, params)
}

// Get retrieves a single CFDI by ID.
func (r *CfdiResource) Get(ctx context.Context, id string) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/cfdi/"+url.PathEscape(id), nil, nil)
}

// GetEstatus queries the SAT status of a CFDI.
func (r *CfdiResource) GetEstatus(ctx context.Context, id string) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/cfdi/"+url.PathEscape(id)+"/estatus", nil, nil)
}

// GetXml downloads the XML of a CFDI.
func (r *CfdiResource) GetXml(ctx context.Context, id string) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/cfdi/"+url.PathEscape(id)+"/xml", nil, nil)
}

// GetPdf downloads the PDF representation of a CFDI.
func (r *CfdiResource) GetPdf(ctx context.Context, id string) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/cfdi/"+url.PathEscape(id)+"/pdf", nil, nil)
}

// GetAcuse retrieves the cancellation acknowledgement (acuse) for a CFDI.
func (r *CfdiResource) GetAcuse(ctx context.Context, id string) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/cfdi/"+url.PathEscape(id)+"/acuse", nil, nil)
}
