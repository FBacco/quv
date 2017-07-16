<?php

namespace AppBundle\Twig;

use AppBundle\FrequencyProvider;

class FrequencyExtension extends \Twig_Extension
{
    /**
     * @var FrequencyProvider
     */
    private $frequencyProvider;

    public function __construct(FrequencyProvider $frequencyProvider)
    {
        $this->frequencyProvider = $frequencyProvider;
    }

    public function getFunctions()
    {
        return [
            new \Twig_SimpleFunction('frequency', [$this->frequencyProvider, 'get']),
        ];
    }
}
