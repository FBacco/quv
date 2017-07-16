<?php

namespace AppBundle\Controller;

use AppBundle\Entity\Record;
use AppBundle\Form\SearchType;
use AppBundle\FrequencyProvider;
use AppBundle\Model\Search;
use Psr\Log\LoggerInterface;
use Sensio\Bundle\FrameworkExtraBundle\Configuration\Route;
use Sensio\Bundle\FrameworkExtraBundle\Configuration\Method;
use Symfony\Bundle\FrameworkBundle\Controller\Controller;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\Response;

class DefaultController extends Controller
{
    /**
     * @Route("/", name="homepage")
     */
    public function indexAction(Request $request)
    {
        $search = new Search();
        $form = $this->container->get('form.factory')->create(SearchType::class, $search, ['method' => 'GET']);
        $form->handleRequest($request);

        $repository = $this->getDoctrine()->getRepository('AppBundle:Record');

        return $this->render('@App/default/index.html.twig', [
            'records'     => $repository->findByDate($search),
            'plotRecords' => $repository->findForPlot($search),
            'search'      => $search,
            'form'        => $form->createView(),
        ]);
    }

    /**
     * @Route("/api/record/{delay}", name="api_record", requirements={"delay"="\d+"})
     * @Method("POST")
     */
    public function recordAction(int $delay, FrequencyProvider $frequencyProvider, LoggerInterface $logger)
    {
        if (0 >= $delay) {
            // Wrong input, do not save and ask for new record sooner than normal frequency
            $logger->error(sprintf('Received "%s" from device, ignoring.', $delay));

            return new Response(sprintf('next=%d', $frequencyProvider->get() / 4), Response::HTTP_BAD_REQUEST);
        }

        $record = new Record($delay);

        $em = $this->getDoctrine()->getManager();
        $em->persist($record);
        $em->flush();

        return new Response(sprintf('next=%d', $frequencyProvider->get()));
    }

    /**
     * @Route("/api/frequency", name="api_frequency")
     * @Method("POST")
     */
    public function frequencyAction(Request $request, FrequencyProvider $frequencyProvider)
    {
        $frequency = $request->request->get('frequency');
        if (null === $frequency) {
            return new Response(1, Response::HTTP_BAD_REQUEST);
        }

        $frequencyProvider->set($frequency);

        return new Response(0);
    }

    /**
     * @Route("/test/liters/{delay}", name="test_liters", requirements={"delay"="\d+"})
     */
    public function computeLitersAction(int $delay)
    {
        $record = new Record($delay);
        $record->computeLiters();

        return new Response($record->getNbLiters());
    }
}
