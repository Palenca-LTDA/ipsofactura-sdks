package ipsofactura

import (
	"context"
	"net/url"
)

// WebhooksResource groups all webhook-related API calls.
type WebhooksResource struct {
	client *Client
}

// List returns all webhooks configured for the account.
func (r *WebhooksResource) List(ctx context.Context) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/webhooks", nil, nil)
}

// Crear creates a new webhook.
func (r *WebhooksResource) Crear(ctx context.Context, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/webhooks", payload, nil)
}

// Actualizar updates an existing webhook.
func (r *WebhooksResource) Actualizar(ctx context.Context, id string, payload map[string]any) (map[string]any, error) {
	return r.client.do(ctx, "PATCH", "/webhooks/"+url.PathEscape(id), payload, nil)
}

// Eliminar deletes a webhook.
func (r *WebhooksResource) Eliminar(ctx context.Context, id string) (map[string]any, error) {
	return r.client.do(ctx, "DELETE", "/webhooks/"+url.PathEscape(id), nil, nil)
}

// ListDeliveries returns the delivery history for a webhook.
func (r *WebhooksResource) ListDeliveries(ctx context.Context, webhookID string) (map[string]any, error) {
	return r.client.do(ctx, "GET", "/webhooks/"+url.PathEscape(webhookID)+"/deliveries", nil, nil)
}

// GetDelivery retrieves a specific delivery attempt.
func (r *WebhooksResource) GetDelivery(ctx context.Context, webhookID, deliveryID string) (map[string]any, error) {
	return r.client.do(ctx, "GET",
		"/webhooks/"+url.PathEscape(webhookID)+"/deliveries/"+url.PathEscape(deliveryID),
		nil, nil)
}

// ResendDelivery re-sends a failed delivery.
func (r *WebhooksResource) ResendDelivery(ctx context.Context, webhookID, deliveryID string) (map[string]any, error) {
	return r.client.do(ctx, "POST",
		"/webhooks/"+url.PathEscape(webhookID)+"/deliveries/"+url.PathEscape(deliveryID)+"/resend",
		nil, nil)
}

// Test sends a test event to the webhook URL.
func (r *WebhooksResource) Test(ctx context.Context, webhookID string) (map[string]any, error) {
	return r.client.do(ctx, "POST", "/webhooks/"+url.PathEscape(webhookID)+"/test", nil, nil)
}
