<?php

namespace AppBundle;

use Psr\Cache\CacheItemInterface;
use Psr\Cache\CacheItemPoolInterface;

class FrequencyProvider
{
    const KEY = 'quv.frequency';

    const DEFAULT_FREQUENCY = 'day';

    const FREQUENCIES = [
        'minute'    => 60,
        'half-hour' => 60*30,
        'hour'      => 60*60,
        'half-day'  => 60*60*12,
        'day'       => 60*60*24,
    ];

    /**
     * @var CacheItemPoolInterface
     */
    private $cache;

    public function __construct(CacheItemPoolInterface $cache)
    {
        $this->cache = $cache;
    }

    public function set(int $frequency)
    {
        $item = $this->getItem();
        $item->set($frequency);

        $this->saveItem($item);
    }

    public function get() : int
    {
        $item = $this->getItem();

        if (!$item->isHit()) {
            $item->set(self::FREQUENCIES[self::DEFAULT_FREQUENCY]);
            $this->saveItem($item);
        }

        return $item->get();
    }

    private function getItem() : CacheItemInterface
    {
        return $this->cache->getItem(self::KEY);
    }

    private function saveItem(CacheItemInterface $item)
    {
        $this->cache->save($item);
    }
}
