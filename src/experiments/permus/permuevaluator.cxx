#include "std.hxx"

#include "Parameters.h"
#include "permuevaluator.h"
#include "map.h"
#include "network.h"
#include "networkexecutor.h"
#include "resource.h"
#include <assert.h>
#include "PBP.h"
//#include "PFSP.h"
#include "LOP.h"
#include "QAP.h"
//#include "TSP.h"
//#include "API.h"
#include "Population.h"
#include "Tools.h"

using namespace std;



namespace NEAT
{
// variables to print progress
static int nnets;
ushort net_idx;
real_t progress_print_decider;

struct Config
{   

};



PBP *GetProblemInfo(string problemType, string filename)
{
    PBP *problem;
    // if (problemType == "pfsp")
    //     problem = new PFSP();
    // else if (problemType == "tsp")
    //     problem = new TSP();
    // else 
    if (problemType == "qap")
        {problem = new QAP();}
    else if (problemType == "lop")
        {problem = new LOP();}
    // else if (problemType == "api")
    //     problem = new API();
    else
    {
         cout << "Wrong problem type was specified. PLease, specify qap, lop, tsp or pfsp" << endl;
         exit(1);
    }

    //Read the instance.
    problem->Read(filename);
    return problem;
}




struct Evaluator
{

    typedef NEAT::Config Config;
    const Config *config;
    bool terminated;
    OrganismEvaluation eval;
    int idx_particle;
    int n_of_repetitions_completed;
    float* v_of_fitness;

    // ushort max_it;


//////////////////////////////////////

    PBP *problem;
    CPopulation *pop;

    // constructor
    __net_eval_decl Evaluator(const Config *config_): config(config_)
    {
        terminated = false;
        eval.error = 0.0;
        eval.fitness = 0.0;
        // it = 0;
        idx_particle = -1;
        n_of_repetitions_completed = 0;
        // max_it = 10;

//////////////////////////////////////

        //char* params[3] = {"binary_name", "lop", "src/experiments/permus/instances/lop/Cebe.lop.n30.1"};
        
        char const *params[3] = {"binary_name", "qap", "src/experiments/permus/instances/qap/tai35a.dat.dat"};
        set_parameters(3, params); // Read parameters from bash.
        //Read the problem instance to optimize.
        problem = GetProblemInfo(PROBLEM_TYPE, INSTANCE_PATH);
        pop = new CPopulation(problem);
        v_of_fitness = new float[REPEATED_EVALUATIONS];



    }

    // Check if evaluation is terminated. If it is, __net_eval_decl OrganismEvaluation result() is called.
    __net_eval_decl bool next_step()
    {
        return !terminated;
    }









    // Clear non-input latent variables. (for example, the states of previous iterations on a recurrent net)
    // This function is called before each iteration, so it could potentially be used to compute the necessary info.
    __net_eval_decl bool clear_noninput()
    {   idx_particle++;
        if (idx_particle == POPSIZE)
        {
            pop->end_iteration();
            terminated = pop->terminated;
        }
        idx_particle = idx_particle % POPSIZE;

        if (terminated)
        {
            v_of_fitness[n_of_repetitions_completed] = pop->f_best;
            if (n_of_repetitions_completed < REPEATED_EVALUATIONS - 1) // another evaluation needed
            {
                n_of_repetitions_completed++;
                terminated = false;
                pop->Reset();
            }else{
                terminated = true;
            }
        }


    return true;
    }

    // load the sensory input
    __net_eval_decl real_t get_sensor(node_size_t sensor_index)
    {
    return pop->pop_info[idx_particle][sensor_index];
    }

    // load the outputs, and assign fitness if it it is the last iteration.
    __net_eval_decl void evaluate(real_t *output)
    {
        if (!terminated)
        {
                pop->apply_neat_output_to_individual_i(output, idx_particle);
        }
    }

    __net_eval_decl OrganismEvaluation result()
    {   
        progress_print_decider += 10.0 / (double) nnets;

        if (net_idx == 0)
        {
            cout << "[" << std::flush;
        }
        
        while (progress_print_decider >= 1.0)
        {
            std::cout << "." << std::flush;
            progress_print_decider--;
        }

        if (net_idx == nnets-1)
        {
            cout << "]" << endl;

        }
        


        net_idx++;  
        //eval.fitness = pop->f_best;
        eval.fitness = Average(v_of_fitness,REPEATED_EVALUATIONS);
        eval.error = 10000000.0-eval.fitness;
        delete pop;
        delete problem;
        return eval;
    }
};

class PermuEvaluator : public NetworkEvaluator
{
    NetworkExecutor<Evaluator> *executor;

public:
    PermuEvaluator()
    {
        executor = NetworkExecutor<Evaluator>::create();
        // Evaluator::Config *config;
        // size_t configlen;
        // create_config(config, configlen);
        // executor->configure(config, configlen);
        // free(config);
    }

    ~PermuEvaluator()
    {
        delete executor;
    }

    virtual void execute(class Network **nets_,
                         class OrganismEvaluation *results,
                         size_t nnets)
    {
        NEAT::nnets = nnets;
        NEAT::net_idx = 0;
        NEAT::progress_print_decider = 0.0;


        executor->execute(nets_, results, nnets);
        
    }
};

class NetworkEvaluator *create_permu_evaluator()
{
    return new PermuEvaluator();
}

} // namespace NEAT
