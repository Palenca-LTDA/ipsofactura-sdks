package ipsofactura

import (
	"context"
	"net/url"
)

// EmpresasResource groups all empresa-related API calls.
type EmpresasResource struct {
	client *Client
}

// List returns all empresas. Supported params: q, include_certificados.
func (r *EmpresasResource) List(ctx context.Context, params url.Values) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/empresas", nil, params)
}

// Crear creates a new empresa.
func (r *EmpresasResource) Crear(ctx context.Context, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/empresas", payload, nil)
}

// Get retrieves an empresa by ID.
func (r *EmpresasResource) Get(ctx context.Context, id string) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/empresas/"+url.PathEscape(id), nil, nil)
}

// Actualizar updates an empresa.
func (r *EmpresasResource) Actualizar(ctx context.Context, id string, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "PATCH", "/empresas/"+url.PathEscape(id), payload, nil)
}

// SubirCertificado uploads a CSD certificate to an empresa.
func (r *EmpresasResource) SubirCertificado(ctx context.Context, empresaID string, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/empresas/"+url.PathEscape(empresaID)+"/certificados", payload, nil)
}

// RefreshPassword updates the password of an empresa's certificate.
func (r *EmpresasResource) RefreshPassword(ctx context.Context, empresaID string, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/empresas/"+url.PathEscape(empresaID)+"/certificados/refresh-password", payload, nil)
}

// EliminarCertificado deletes a certificate by serial number.
func (r *EmpresasResource) EliminarCertificado(ctx context.Context, empresaID, numero string) (map[string]any, error) {
	return r.client.do(ctx, "DELETE", "/empresas/"+url.PathEscape(empresaID)+"/certificados/"+url.PathEscape(numero), nil, nil)
}
