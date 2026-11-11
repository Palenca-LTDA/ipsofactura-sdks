// Package ipsofactura is the official Go client for the Ipsofactura API.
// It allows you to stamp CFDI 4.0, manage empresas, and configure webhooks.
//
// Usage:
//
//	client := ipsofactura.NewClient("your_api_key")
//	// or sandbox:
//	client := ipsofactura.NewClient("your_api_key", ipsofactura.WithBaseURL(ipsofactura.SandboxURL))
package ipsofactura

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"time"
)

const (
	SandboxURL = "https://sandbox.api.ipsofactura.com"
	ProdURL    = "https://api.ipsofactura.com"
)

// IpsofacturaError is returned when the API responds with HTTP 4xx or 5xx.
type IpsofacturaError struct {
	StatusCode int
	Code       string `json:"code"`
	Message    string `json:"message"`
	SatCode    string `json:"sat_code"`
	RetryAfter int    `json:"retryAfter"`
}

func (e *IpsofacturaError) Error() string {
	msg := fmt.Sprintf("ipsofactura: HTTP %d [%s] %s", e.StatusCode, e.Code, e.Message)
	if e.SatCode != "" {
		msg += " (SAT: " + e.SatCode + ")"
	}
	return msg
}

// Option is a functional option for configuring the Client.
type Option func(*Client)

// WithBaseURL overrides the default base URL (use SandboxURL for testing).
func WithBaseURL(baseURL string) Option {
	return func(c *Client) {
		c.baseURL = baseURL
	}
}

// WithHTTPClient replaces the default HTTP client.
func WithHTTPClient(hc *http.Client) Option {
	return func(c *Client) {
		c.httpClient = hc
	}
}

// Client is the main Ipsofactura API client.
type Client struct {
	apiKey     string
	baseURL    string
	httpClient *http.Client

	Cfdi     *CfdiResource
	Empresas *EmpresasResource
	Webhooks *WebhooksResource
}

// NewClient creates a new Ipsofactura client.
// By default it targets the production URL.
func NewClient(apiKey string, opts ...Option) *Client {
	c := &Client{
		apiKey:  apiKey,
		baseURL: ProdURL,
		httpClient: &http.Client{
			Timeout: 30 * time.Second,
		},
	}
	for _, opt := range opts {
		opt(c)
	}
	c.Cfdi = &CfdiResource{client: c}
	c.Empresas = &EmpresasResource{client: c}
	c.Webhooks = &WebhooksResource{client: c}
	return c
}

// do executes an HTTP request and decodes the JSON response.
// On HTTP >= 400 it returns an *IpsofacturaError.
func (c *Client) do(ctx context.Context, method, path string, body map[string]any, query url.Values) (map[string]any, error) {
	fullURL := c.baseURL + path
	if len(query) > 0 {
		fullURL += "?" + query.Encode()
	}

	var reqBody io.Reader
	if body != nil {
		b, err := json.Marshal(body)
		if err != nil {
			return nil, fmt.Errorf("ipsofactura: marshal body: %w", err)
		}
		reqBody = bytes.NewReader(b)
	}

	req, err := http.NewRequestWithContext(ctx, method, fullURL, reqBody)
	if err != nil {
		return nil, fmt.Errorf("ipsofactura: create request: %w", err)
	}

	req.Header.Set("x-api", c.apiKey)
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Accept", "application/json")

	resp, err := c.httpClient.Do(req)
	if err != nil {
		return nil, fmt.Errorf("ipsofactura: http: %w", err)
	}
	defer resp.Body.Close()

	respBytes, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("ipsofactura: read response: %w", err)
	}

	var result map[string]any
	if len(respBytes) > 0 {
		if err := json.Unmarshal(respBytes, &result); err != nil {
			// non-JSON body — wrap as raw
			result = map[string]any{"raw": string(respBytes)}
		}
	}

	if resp.StatusCode >= 400 {
		apiErr := &IpsofacturaError{StatusCode: resp.StatusCode}
		if v, ok := result["code"].(string); ok {
			apiErr.Code = v
		}
		if v, ok := result["message"].(string); ok {
			apiErr.Message = v
		}
		if v, ok := result["sat_code"].(string); ok {
			apiErr.SatCode = v
		}
		if v, ok := result["retryAfter"].(float64); ok {
			apiErr.RetryAfter = int(v)
		}
		return nil, apiErr
	}

	return result, nil
}
