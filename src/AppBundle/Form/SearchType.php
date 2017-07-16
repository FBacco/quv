<?php

namespace AppBundle\Form;

use AppBundle\Repository\RecordRepository;
use Symfony\Component\Form\AbstractType;
use Symfony\Component\Form\Extension\Core\Type\ChoiceType;
use Symfony\Component\Form\Extension\Core\Type\DateType;
use Symfony\Component\Form\FormBuilderInterface;
use Symfony\Component\OptionsResolver\OptionsResolver;

class SearchType extends AbstractType
{
    public function buildForm(FormBuilderInterface $builder, array $options)
    {
        $builder
            ->add('from', DateType::class, [
                'widget' => 'single_text',
            ])
            ->add('to', DateType::class, [
                'widget' => 'single_text',
            ])
            ->add('step', ChoiceType::class, [
                'choices'      => array_keys(RecordRepository::GROUPBY),
                'choice_label' => function ($value) {
                    return $value;
                },
            ])
        ;
    }

    public function configureOptions(OptionsResolver $resolver)
    {
        $resolver->setDefaults([
            'csrf_protection' => false
        ]);
    }

    public function getBlockPrefix()
    {
        return '';
    }
}
