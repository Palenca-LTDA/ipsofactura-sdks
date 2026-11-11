package ipsofactura

import (
	"crypto/hmac"
	"crypto/sha256"
	"encoding/hex"
	"strings"
)

// VerifyWebhookSignature validates the HMAC-SHA256 signature of an incoming webhook event.
//
// The signed message is: timestamp + "." + payload.
// The signature is expected as a lowercase hex string.
//
// Returns true if the signature is valid.
func VerifyWebhookSignature(payload, timestamp, signature, secret string) bool {
	message := timestamp + "." + payload
	mac := hmac.New(sha256.New, []byte(secret))
	mac.Write([]byte(message))
	expected := hex.EncodeToString(mac.Sum(nil))
	return hmac.Equal([]byte(expected), []byte(strings.ToLower(signature)))
}
