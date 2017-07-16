<?php

namespace AppBundle\Model;

class Search
{
    /**
     * @var \DateTime
     */
    private $from;

    /**
     * @var \DateTime
     */
    private $to;

    /**
     * @var string
     */
    private $step = 'day';

    public function __construct()
    {
        $this->from = new \DateTime('-7 days');
        $this->to   = new \DateTime('tomorrow');
    }

    public function setFrom(\DateTime $from)
    {
        $this->from = $from;
    }

    public function getFrom()
    {
        return $this->from;
    }

    public function setTo(\DateTime $to)
    {
        $this->to = $to;
    }

    public function getTo()
    {
        return $this->to;
    }

    public function setStep(string $step)
    {
        $this->step = $step;
    }

    public function getStep()
    {
        return $this->step;
    }
}
