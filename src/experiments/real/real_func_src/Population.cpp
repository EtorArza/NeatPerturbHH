#include "Population.h"
#include "MultidimBenchmarkFF.h"
#include "Parameters.h"
#include "Tools.h"
#include "Tabu.h"
#include "../real_func_evaluator.h"
#include "../REAL_FUNC_params.h"
#include <assert.h>
#include <float.h>

using std::cerr;
using std::cout;
using std::endl;
using std::istream;
using std::ostream;


void CPopulation::init_class(MultidimBenchmarkFF *problem, RandomNumberGenerator* rng, REAL_FUNC::params* parameters)
{


    assert(parameters->SOLVER_POPSIZE > 0);
    assert(problem->GetProblemSize() > 0);
    assert(parameters->MAX_SOLVER_FE > 0);


    this->rng = rng;
    this->problem = problem;
    this->popsize = parameters->SOLVER_POPSIZE;
    this->n = problem->GetProblemSize();
    this->n_evals = 0;
    this->max_evals = parameters->MAX_SOLVER_FE;
    genome_best = new double[n];
    f_best = -DBL_MAX;
    GenerateRandomRealvec_0_1(this->genome_best, n, this->rng);
    templ_double_array1_of_size_n = new double[n];
    templ_double_array2_of_size_n = new double[n];
    templ_double_array1_of_size_POPSIZE = new double[this->popsize];
    templ_double_array2_of_size_POPSIZE = new double[this->popsize];



    m_individuals.resize(popsize);

    pop_info = new double *[popsize];

    //Initialize population with random solutions
    for (int i = 0; i < popsize; i++)
    {
        m_individuals[i] = new CIndividual(n, this->rng);
    }

    for (int i = 0; i < popsize; i++)
    {
        pop_info[i] = new double[REAL_FUNC::__sensor_N];
    }

    terminated = false;
    evaluate_population();
    end_iteration();
}

CPopulation::CPopulation(MultidimBenchmarkFF *problem, REAL_FUNC::params* parameters)
{
    RandomNumberGenerator* tmp_rng = new RandomNumberGenerator();
    init_class(problem, tmp_rng, parameters);
}


CPopulation::CPopulation(MultidimBenchmarkFF *problem, RandomNumberGenerator* rng, REAL_FUNC::params* parameters)
{
    init_class(problem, rng, parameters);
}

void CPopulation::Reset()
{
    f_best = -DBL_MAX;
    GenerateRandomRealvec_0_1(this->genome_best, n, rng);
    for (int i = 0; i < popsize; i++)
    {
        auto tmp = std::vector<double>();
        std::swap(tmp, m_individuals[i]->activation);
        m_individuals[i]->reset(rng);
        std::swap(tmp, m_individuals[i]->activation);
    }
    terminated = false;
    evaluate_population();
    this->n_evals = 0;
    end_iteration();
}


/*
 * Destructor function.
 */
CPopulation::~CPopulation()
{
    for (int i = 0; i < this->popsize; i++)
    {
        delete[] pop_info[i];
    }
    for (int i = 0; i < this->popsize; i++)
    {
        delete m_individuals[i];
    }
    
    m_individuals.clear();
    delete rng;
    rng=NULL;    
    delete[] pop_info;
    pop_info=NULL;
    delete[] genome_best;
    genome_best=NULL;
    delete[] templ_double_array1_of_size_n;
    delete[] templ_double_array2_of_size_n;
    templ_double_array1_of_size_n=NULL;
    templ_double_array2_of_size_n=NULL;
    delete[] templ_double_array1_of_size_POPSIZE;
    templ_double_array1_of_size_POPSIZE=NULL;
    delete[] templ_double_array2_of_size_POPSIZE;
    templ_double_array2_of_size_POPSIZE=NULL;
}



void CPopulation::end_iteration(){
    n_evals += this->popsize;
    SortPopulation();
    get_population_info();
    if(n_evals > this->max_evals)
    {
        terminated = true;
    }
    //PrintMatrix(pop_info, POPSIZE, NEAT::__sensor_N);
}




double *CPopulation::get_neat_input_individual_i(int i)
{
    return pop_info[i];
}

void CPopulation::apply_neat_output_to_individual_i(double *output_neat, int i)
{

    #define MAX_COMPONENT_STEP_SIZE 1

    
    for (int j = 0; j < n; j++)
    {
        templ_double_array1_of_size_n[j] = m_individuals[i]->genome_best[j] - m_individuals[i]->genome[j];
        templ_double_array2_of_size_n[j] = this->genome_best[j] - m_individuals[i]->genome[j];
    }


    normalize_vector_L1(m_individuals[i]->momentum, n);
    normalize_vector_L1(m_individuals[i]->momentum, m_individuals[i]->n);
    normalize_vector_L1(m_individuals[i]->momentum, m_individuals[i]->n);



    for (int j = 0; j < n; j++)
    {
        m_individuals[i]->momentum[j] = MAX_COMPONENT_STEP_SIZE * 
        (
            m_individuals[i]->momentum[j] * output_neat[REAL_FUNC::MOMENTUM] +
            templ_double_array1_of_size_n[j] * output_neat[REAL_FUNC::L_BEST] + 
            templ_double_array2_of_size_n[j] * output_neat[REAL_FUNC::G_BEST] 
        );
    }




    // clip momentum  between -1 and 1
    for (int j = 0; j < n; j++)
    {
        m_individuals[i]->momentum[j] = max(m_individuals[i]->momentum[j], -1.0);
        m_individuals[i]->momentum[j] = min(m_individuals[i]->momentum[j], 1.0);
    }


    sum_arrays(m_individuals[i]->genome, m_individuals[i]->genome, m_individuals[i]->momentum, n);



    m_individuals[i]->amount_clipped_last_it = 0.0;
    // clip values between 0 and 1
    for (int j = 0; j < n; j++)
    {   
        // double non_clipped_value = m_individuals[i]->genome[j];
        m_individuals[i]->genome[j] = max(m_individuals[i]->genome[j], 0.0);
        m_individuals[i]->genome[j] = min(m_individuals[i]->genome[j], 1.0);
        // m_individuals[i]->amount_clipped_last_it += abs(non_clipped_value - m_individuals[i]->genome[j]);
    }

    //cout << m_individuals[i]->amount_clipped_last_it << endl;

    this->problem->Evaluate(m_individuals[i]);

    double new_f = m_individuals[i]->f_value;

    global_best_was_improved = false;
    if (new_f > this->f_best)
    {
        this->f_best = new_f;
        copy_array(this->genome_best, m_individuals[i]->genome, n);
    }
    global_best_was_improved = true;
}

/*
 * Sorts the individuals in the population in decreasing order of the fitness value.
 */
void CPopulation::SortPopulation()
{
    sort(m_individuals.begin(), m_individuals.end(), Better);
    this->global_best_was_improved = false;
    //cout << this->f_best << "  |  " << m_individuals[0]->f_value << endl;
    if (m_individuals[0]->f_value > this->f_best)
    {
        this->global_best_was_improved = true;
        this->f_best = m_individuals[0]->f_value;
        copy_array(this->genome_best, m_individuals[0]->genome, n);
    }

}

// Evaluate the whole population
void CPopulation::evaluate_population()
{
    for (int i = 0; i < popsize; i++)
    {
        problem->Evaluate(m_individuals[i]);
    }
}



void CPopulation::get_population_info(){
    comp_distance_to_closest();
    comp_dist_to_average();
    comp_relative_dist_to_best();
    comp_relative_time();
    comp_relative_score();
    load_INDIVIDUAL_BEST_WAS_IMPROVED();
    load_GLOBAL_BEST_WAS_IMPROVED();
    load_RANDOM_NUMBER();
}



void CPopulation::comp_distance_to_closest()
{
    for (int i = 0; i < this->popsize; i++)
    {
        double min_dist = DBL_MAX;
        for (int j = 0; j < this->popsize; j++)
        {
            if (i == j)
            {
                continue;
            }
            double d = euclid_dist(m_individuals[i]->genome, m_individuals[j]->genome, n);
            if (d < min_dist)
            {
                min_dist = d;
            }
        }
        templ_double_array1_of_size_POPSIZE[i] = min_dist;
    }
    compute_order_from_double_to_double(templ_double_array1_of_size_POPSIZE, this->popsize, templ_double_array2_of_size_POPSIZE);
    multiply_array_with_value(templ_double_array2_of_size_POPSIZE, 1/ (double) this->popsize, this->popsize);
    for (int i = 0; i < this->popsize; i++)
    {
        pop_info[i][REAL_FUNC::DIST_TO_CLOSEST] = templ_double_array2_of_size_POPSIZE[i];
    }
}



void CPopulation::comp_dist_to_average()
{
    for (int j = 0; j < n; j++)
    {
        templ_double_array1_of_size_n[j] = 0.0;
        for (int i = 0; i < this->popsize; i++)
        {
            templ_double_array1_of_size_n[j] += m_individuals[i]->genome[j];
        }
        templ_double_array1_of_size_n[j] /= (double) this->popsize;
    }
    for (int i = 0; i < this->popsize; i++)
    {
        templ_double_array2_of_size_POPSIZE[i] = euclid_dist(templ_double_array1_of_size_n, m_individuals[i]->genome, n);
    }

    compute_order_from_double_to_double(templ_double_array2_of_size_POPSIZE, this->popsize, templ_double_array1_of_size_POPSIZE);
    multiply_array_with_value(templ_double_array1_of_size_POPSIZE, 1/ (double) this->popsize, this->popsize);

    for (int i = 0; i < this->popsize; i++)
    {
        pop_info[i][REAL_FUNC::DIST_TO_AVERAGE] = templ_double_array1_of_size_POPSIZE[i];
    }

}


// relative to best in population.
void CPopulation::comp_relative_dist_to_best()
{
    for (int j = 0; j < this->popsize; j++)
    {
        templ_double_array2_of_size_POPSIZE[j] = euclid_dist(m_individuals[0]->genome, m_individuals[j]->genome, n);
    }
    compute_order_from_double_to_double(templ_double_array2_of_size_POPSIZE, this->popsize, templ_double_array1_of_size_POPSIZE);
    multiply_array_with_value(templ_double_array1_of_size_POPSIZE, 1/ (double) this->popsize, this->popsize);

    for (int i = 0; i < this->popsize; i++)
    {
        pop_info[i][REAL_FUNC::RELATIVE_DIST_TO_BEST] = templ_double_array1_of_size_POPSIZE[i];
    }

}


void CPopulation::comp_relative_time()
{
    for (int i = 0; i < this->popsize; i++)
    {
        pop_info[i][REAL_FUNC::RELATIVE_TIME] = (double) n_evals / (double) this->max_evals;
    }
    return ;
}



void CPopulation::comp_relative_score()
{
    for (int i = 0; i < this->popsize; i++)
    {
        double res =  (double)i / (double)this->popsize;
        this->m_individuals[i]->relative_pos = res;
        pop_info[i][REAL_FUNC::RELATIVE_SCORE] = res;
    }
}


void CPopulation::load_INDIVIDUAL_BEST_WAS_IMPROVED()
{
    for (int i = 0; i < this->popsize; i++)
    {
        pop_info[i][REAL_FUNC::INDIVIDUAL_BEST_WAS_IMPROVED] = (double) this->m_individuals[i]->bk_was_improved;
    }
}




void CPopulation::load_GLOBAL_BEST_WAS_IMPROVED()
{
    for (int i = 0; i < this->popsize; i++)
    {
        pop_info[i][REAL_FUNC::GLOBAL_BEST_WAS_IMPROVED] = (double) this->global_best_was_improved;
    }
}


void CPopulation::load_RANDOM_NUMBER(){
    for (int i = 0; i < this->popsize; i++)
    {
        pop_info[i][REAL_FUNC::RANDOM_NUMBER] = (double) this->rng->random_0_1_double();
    }
}



void CPopulation::Print()
{
    PrintMatrix(pop_info,this->popsize, REAL_FUNC::__sensor_N);
    cout << "----------" << endl;
}

void CPopulation::print_positions(std::string filename){
    std::string res_to_write = "[";
    for (int i = 0; i < this->popsize; i++)
    {   
        res_to_write += "[";
        for (int j = 0; j < this->problem->GetProblemSize(); j++)
        {
            res_to_write += std::to_string(m_individuals[i]->genome[j]);
            if (j != this->problem->GetProblemSize() - 1 )
            {
                res_to_write += ",";
            }
        }
        res_to_write += "]";
        if (i != this->popsize - 1 )
        {
            res_to_write += ",";
        }
    }
    res_to_write += "]\n";
    append_line_to_file(filename, res_to_write);
}