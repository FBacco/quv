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
     *
     * @ORM\Column(type="datetime")
     */
    private $recordedAt;

    /**
     * @var int
     *
     * @ORM\Column(type="integer")
     */
    private $delay;

    /**
     * @var int
     *
     * @ORM\Column(type="integer")
     */
    private $nbLiters;

    /**
     * @var int
     *
     * @ORM\Column(type="integer")
     */
    private $temperature;

    /**
     * @var int
     *
     * @ORM\Column(type="integer")
     */
    private $humidity;


    public function __construct(int $delay, int $temperature, int $humidity)
    {
        $this->recordedAt = new \DateTime();
        $this->delay      = $delay;
        $this->temperature = $temperature;
        $this->humidity = $humidity;
    }

    /**
     * @ORM\PrePersist
     * @ORM\PreUpdate
     */
    public function computeLiters()
    {
        $process = new Process(sprintf('..\\quv.exe dt=%s -s', $this->delay));
        $process->mustRun();

        $this->nbLiters = (int) $process->getOutput();
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

    public function getRecordedAt() : \DateTime
    {
        return $this->recordedAt;
    }

    public function getDelay() : int
    {
        return $this->delay;
    }

    public function getNbLiters() : int
    {
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
}
