<?php

declare(strict_types=1);

namespace Ipsofactura;

class IpsofacturaException extends \RuntimeException
{
    public function __construct(
        private int $statusCode,
        public readonly string $code,
        string $message,
        public readonly ?string $satCode = null,
        public readonly ?int $retryAfter = null,
    ) {
        parent::__construct($message, $statusCode);
    }

    public function getStatusCode(): int
    {
        return $this->statusCode;
    }
}
