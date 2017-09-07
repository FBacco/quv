<?php

namespace AppBundle\Entity;

use Doctrine\ORM\Mapping as ORM;
use Symfony\Component\Process\Process;

/**
 * @ORM\Entity(repositoryClass="AppBundle\Repository\RecordRepository")
 * @ORM\HasLifecycleCallbacks()
 */
class Record
{
    /**
     * @var int
     *
     * @ORM\Column(name="id", type="integer")
     * @ORM\Id
     * @ORM\GeneratedValue(strategy="AUTO")
     */
    private $id;

    /**
     * @var \DateTime
     * @ORM\Column(type="datetime")
     */
    private $recordedAt;

    /**
     * @var int
     * @ORM\Column(type="integer", nullable=true)
     */
    private $delay;

    /**
     * @var int
     * @ORM\Column(type="float", scale=2, nullable=true)
     */
    private $distance;

    /**
     * @var int
     * @ORM\Column(type="integer", nullable=true)
     */
    private $nbLiters;

    /**
     * @var int
     * @ORM\Column(type="integer", nullable=true)
     */
    private $temperature;

    /**
     * @var int
     * @ORM\Column(type="integer", nullable=true)
     */
    private $humidity;

    /**
     * @var int
     * @ORM\Column(type="integer", nullable=true)
     */
    private $rssi;


    public function __construct(
        int $delay = null,
        int $temperature = null,
        int $humidity = null, 
        int $rssi = null
    ) {
        $this->recordedAt  = new \DateTime();
        $this->delay       = $delay;
        $this->distance    = null;
        $this->nbLiters    = null;
        $this->temperature = $temperature;
        $this->humidity    = $humidity;
        $this->rssi        = $rssi;
    }

    /**
     * @ORM\PrePersist
     * @ORM\PreUpdate
     */
    public function computeLiters()
    {
        if (null === $this->delay) return;
        
        $process = new Process(sprintf('..\\quv.exe dt=%s -s', $this->delay));
        $process->mustRun();
        $output = explode(' ', $process->getOutput());
        
        $this->nbLiters = (int)$output[0];
        $this->distance = (float)$output[1];
    }

    public function debugLiters()
    {
        $process = new Process(sprintf('..\\quv.exe dt=%s', $this->delay));
        $process->run();
        return $process->getOutput();
    }

    public function getId()
    {
        return $this->id;
    }

    /**
     * @return DateTime
     */
    public function getRecordedAt() : \DateTime
    {
        return $this->recordedAt;
    }

    /**
     * @return int
     */
    public function getDelay() : int
    {
        return $this->delay;
    }

    /**
     * @return int
     */
    public function getDistance() : int
    {
        return $this->distance;
    }

    /**
     * @return int
     */
    public function getNbLiters() : int
    {
        if (null === this->nbLiters) {
            $this->computeLiters();
        }
        return $this->nbLiters;
    }

    /**
     * @return int
     */
    public function getTemperature(): int
    {
        return $this->temperature;
    }

    /**
     * @return int
     */
    public function getHumidity(): int
    {
        return $this->humidity;
    }

    /**
     * @return int
     */
    public function getRssi(): int
    {
        return $this->rssi;
    }
}
