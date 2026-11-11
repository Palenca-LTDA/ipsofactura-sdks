package com.ipsofactura;

/**
 * Exception thrown when the Ipsofactura API returns an error response.
 */
public class IpsofacturaException extends RuntimeException {

    private final int statusCode;
    private final String code;
    private final String satCode;
    private final Integer retryAfter;

    public IpsofacturaException(int statusCode, String code, String message, String satCode, Integer retryAfter) {
        super(message);
        this.statusCode = statusCode;
        this.code = code;
        this.satCode = satCode;
        this.retryAfter = retryAfter;
    }

    /** HTTP status code returned by the API (e.g. 400, 422, 500). */
    public int getStatusCode() {
        return statusCode;
    }

    /** Machine-readable error code from the API (e.g. "VALIDATION_ERROR"). */
    public String getCode() {
        return code;
    }

    /**
     * SAT CFDI error code when the error originates from the PAC/SAT
     * (e.g. "CFDI40xxx"). May be null for non-SAT errors.
     */
    public String getSatCode() {
        return satCode;
    }

    /**
     * Suggested number of seconds to wait before retrying the request.
     * May be null when the error is not rate-limiting in nature.
     */
    public Integer getRetryAfter() {
        return retryAfter;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("IpsofacturaException{");
        sb.append("statusCode=").append(statusCode);
        sb.append(", code='").append(code).append('\'');
        sb.append(", message='").append(getMessage()).append('\'');
        if (satCode != null)   sb.append(", satCode='").append(satCode).append('\'');
        if (retryAfter != null) sb.append(", retryAfter=").append(retryAfter);
        sb.append('}');
        return sb.toString();
    }
}
