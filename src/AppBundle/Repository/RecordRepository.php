<?php

namespace AppBundle\Repository;

use AppBundle\Entity\Record;
use AppBundle\Model\Search;
use Doctrine\ORM\EntityRepository;
use Doctrine\ORM\QueryBuilder;
use Webmozart\Assert\Assert;

class RecordRepository extends EntityRepository
{
    const GROUPBY = [
        'minute' => '%d/%m/%Y %Hh%i',
        'hour'   => '%d/%m/%Y %Hh',
        'day'    => '%d/%m/%Y',
        'week'   => 'S%u %m/%Y',
        'month'  => '%m/%Y',
        'year'   => '%Y',
    ];

    /**
     * @return Record[]
     */
    public function findByDate(Search $search)
    {
        $qb = $this->createQueryBuilder('r')
            ->orderBy('r.recordedAt', 'ASC')
        ;

        $this->filterByDate($qb, $search->getFrom(), $search->getTo());

        return $qb->getQuery()->getResult();
    }

    public function findForPlot(Search $search)
    {
        Assert::keyExists(self::GROUPBY, $search->getStep());

        $qb = $this->createQueryBuilder('r')
            ->select('DATE_FORMAT(r.recordedAt, :groupby) AS step')
            ->addSelect('AVG(r.nbLiters) AS avgLiters')
            ->addSelect('AVG(r.temperature) AS avgTemperature')
            ->addSelect('AVG(r.humidity) AS avgHumidity')
            ->setParameter('groupby', self::GROUPBY[$search->getStep()])
            ->groupby('step')
            ->orderBy('r.recordedAt', 'ASC')
        ;

        $this->filterByDate($qb, $search->getFrom(), $search->getTo());

        return $qb->getQuery()->getResult();
    }

    private function filterByDate(QueryBuilder $qb, \DateTime $from, \DateTime $to)
    {
        $qb
            ->andWhere('r.recordedAt BETWEEN :from AND :to')
            ->setParameter('from', $from)
            ->setParameter('to', $to)
        ;
    }
}
