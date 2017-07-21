<?php

namespace AppBundle\Controller;

use AppBundle\Entity\Record;
use AppBundle\Form\SearchType;
use AppBundle\FrequencyProvider;
use AppBundle\Model\Search;
use Doctrine\ORM\EntityManager;
use Doctrine\ORM\EntityRepository;
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
     * @Route("/api/record", name="api_record")
     * @Method("GET")
     */
    public function recordAction(Request $request, FrequencyProvider $frequencyProvider, LoggerInterface $logger)
    {
        $delay = $request->query->get('delay', null);
        $temperature = $request->query->get('temp', null);
        $humidity = $request->query->get('humidity', null);

        $error = false;

        $error |= !is_numeric($delay) || $delay > 5970 || 0 >= $delay ;
        $error |= !is_numeric($temperature);
        $error |= !is_numeric($humidity);

        if ($error) {
            // Wrong input, do not save and ask for new record sooner than normal frequency
            $logger->error(sprintf(
				'Received "%s" (%x), "%s" (%x), "%s" (%x) from device, ignoring.',
				$delay,
				!is_numeric($delay) || $delay > 5970 || 0 >= $delay  ,
				$temperature,
				!is_numeric($temperature),
				$humidity,
				!is_numeric($humidity)
			));	

            return new Response(sprintf('next=%d', $frequencyProvider->get() / 4), Response::HTTP_BAD_REQUEST);
        }

        $record = new Record($delay, $temperature, $humidity);

        /** @var EntityManager $em */
        $em = $this->getDoctrine()->getManager();
        $em->persist($record);
        $em->flush();

        // Return delay to next wake up, in seconds
        return new Response(sprintf('next=%d\n', 60 * $frequencyProvider->get()));
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

        return new Response(null, 200);
    }

    /**
     * @Route("/test/liters/{delay}", name="test_liters", requirements={"delay"="\d+"})
     */
    public function computeLitersAction(int $delay)
    {
        $record = new Record($delay, 0, 0);
        return new Response(nl2br($record->debugLiters()));
    }
}
