#!/bin/bash

source scripts/array_to_string_functions.sh


COMPILE_JOB_ID=`sbatch --parsable scripts/make_hip.sh --exclude=n[001-004,017-018]`

SRCDIR=`pwd`



if false; then
echo "this part not executed"
# ... Code I want to skip here ...
fi





######################## LOOCV TRAINING #################################
EXPERIMENT_FOLDER_NAME="src/experiments/real/results/comparison_other_pso/controllers_trained_on_all_problems"
SEED=2
NEAT_POPSIZE=512
SOLVER_POPSIZE=20
MAX_SOLVER_FE=100000
MAX_TRAIN_ITERATIONS=2000


instance_list="[1,2,3,4,5,6,7,8,9,11,1,2,3,4,5,6,7,8,9,11]"
dim_list="[10,10,10,10,10,10,10,10,10,10,20,20,20,20,20,20,20,20,20,20]"
lower_list="[-10,-10,-5,-5.12,-15,-600,-5,-15,-100,-2.048,-10,-10,-5,-5.12,-15,-600,-5,-15,-100,-2.048]"
upper_list="[5,10,10,5,30,600,10,30,100,2.048,5,10,10,5,30,600,10,30,100,2.048]"
########################################################################


i=-1


i=$((i+1))
CONTROLLER_NAME_PREFIX_ARRAY+=("trained_with_all_problems")
SEED_ARRAY+=("${SEED}")
COMMA_SEPARATED_PROBLEM_INDEX_LIST_ARRAY+=(`python -c "a = ${instance_list}; a = [str(el) for el in a]; print('s_e_p'.join(a))"`)
COMMA_SEPARATED_PROBLEM_DIM_LIST_ARRAY+=(`python -c "a = ${dim_list}; a = [str(el) for el in a]; print('s_e_p'.join(a))"`)
COMMA_SEPARATED_X_LOWER_LIST_ARRAY+=(`python -c "a = ${lower_list}; a = [str(el) for el in a]; print('s_e_p'.join(a))"`)
COMMA_SEPARATED_X_UPPER_LIST_ARRAY+=(`python -c "a = ${upper_list}; a = [str(el) for el in a]; print('s_e_p'.join(a))"`)




for instance_index in 1 2 3 4 5 6 7 8 9 11; do
    i=$((i+1))

    CONTROLLER_NAME_PREFIX_ARRAY+=("LeaveOutF_${instance_index}")
    SEED_ARRAY+=("${SEED}")

    COMMA_SEPARATED_PROBLEM_INDEX_LIST_ARRAY+=(`python -c "a = ${instance_list}; a.pop(${i}+10); a.pop(${i}); a = [str(el) for el in a]; print('s_e_p'.join(a))"`)
    COMMA_SEPARATED_PROBLEM_DIM_LIST_ARRAY+=(`python -c "a = ${dim_list}; a.pop(${i}+10); a.pop(${i}); a = [str(el) for el in a]; print('s_e_p'.join(a))"`)
    COMMA_SEPARATED_X_LOWER_LIST_ARRAY+=(`python -c "a = ${lower_list}; a.pop(${i}+10); a.pop(${i}); a = [str(el) for el in a]; print('s_e_p'.join(a))"`)
    COMMA_SEPARATED_X_UPPER_LIST_ARRAY+=(`python -c "a = ${upper_list}; a.pop(${i}+10); a.pop(${i}); a = [str(el) for el in a]; print('s_e_p'.join(a))"`)
done




CONTROLLER_NAME_PREFIX_ARRAY=$(to_list "${CONTROLLER_NAME_PREFIX_ARRAY[@]}")
SEED_ARRAY=$(to_list "${SEED_ARRAY[@]}")
COMMA_SEPARATED_PROBLEM_INDEX_LIST_ARRAY=$(to_list "${COMMA_SEPARATED_PROBLEM_INDEX_LIST_ARRAY[@]}")
COMMA_SEPARATED_PROBLEM_DIM_LIST_ARRAY=$(to_list "${COMMA_SEPARATED_PROBLEM_DIM_LIST_ARRAY[@]}")
COMMA_SEPARATED_X_LOWER_LIST_ARRAY=$(to_list "${COMMA_SEPARATED_X_LOWER_LIST_ARRAY[@]}")
COMMA_SEPARATED_X_UPPER_LIST_ARRAY=$(to_list "${COMMA_SEPARATED_X_UPPER_LIST_ARRAY[@]}")




sbatch --parsable --dependency=afterok:${COMPILE_JOB_ID} --export=\
NEAT_POPSIZE=${NEAT_POPSIZE},\
SOLVER_POPSIZE=${SOLVER_POPSIZE},\
MAX_SOLVER_FE=${MAX_SOLVER_FE},\
MAX_TRAIN_ITERATIONS=${MAX_TRAIN_ITERATIONS},\
EXPERIMENT_FOLDER_NAME=${EXPERIMENT_FOLDER_NAME},\
CONTROLLER_NAME_PREFIX_ARRAY=${CONTROLLER_NAME_PREFIX_ARRAY},\
SEED_ARRAY=${SEED_ARRAY},\
COMMA_SEPARATED_PROBLEM_INDEX_LIST_ARRAY=${COMMA_SEPARATED_PROBLEM_INDEX_LIST_ARRAY},\
COMMA_SEPARATED_PROBLEM_DIM_LIST_ARRAY=${COMMA_SEPARATED_PROBLEM_DIM_LIST_ARRAY},\
COMMA_SEPARATED_X_LOWER_LIST_ARRAY=${COMMA_SEPARATED_X_LOWER_LIST_ARRAY},\
COMMA_SEPARATED_X_UPPER_LIST_ARRAY=${COMMA_SEPARATED_X_UPPER_LIST_ARRAY} \
--array=0-$i src/experiments/real/scripts/hip_train_array.sl



