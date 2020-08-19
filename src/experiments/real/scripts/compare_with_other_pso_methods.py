import pandas as pd
import subprocess

CONTROLLER_PATH = "src/experiments/real/results/trained_controllers/top_controllers/controller_1_2_3_best.controller"
N_EVALS = 1
N_REPS = 1
THREADS = 1


def write_conf_file(METHOD_SHORTNAME,SOLVER_POPSIZE,PROBLEM_INDEX,PROBLEM_DIM,MAX_SOLVER_FE,X_LOWER_LIM,X_UPPER_LIM,F_COMPETITION):

    values = {
        "SOLVER_POPSIZE":SOLVER_POPSIZE,
        "PROBLEM_INDEX":PROBLEM_INDEX,
        "PROBLEM_DIM":PROBLEM_DIM,
        "MAX_SOLVER_FE":MAX_SOLVER_FE,
        "X_LOWER_LIM":X_LOWER_LIM,
        "X_UPPER_LIM":X_UPPER_LIM,
        "CONTROLLER_PATH":CONTROLLER_PATH,
        "N_EVALS":N_EVALS,
        "N_REPS":N_REPS,
        "THREADS":THREADS,
    }

    tmp_ini_file_string = """
[Global] 
mode = test
PROBLEM_NAME = real_func

THREADS = {THREADS}
N_EVALS = {N_EVALS}
N_REPS = {N_REPS}
CONTROLLER_PATH = {CONTROLLER_PATH}


PRINT_POSITIONS = false


SOLVER_POPSIZE = {SOLVER_POPSIZE}


PROBLEM_INDEX = {PROBLEM_INDEX}
PROBLEM_DIM = {PROBLEM_DIM}
MAX_SOLVER_FE = {MAX_SOLVER_FE}

X_LOWER_LIM = {X_LOWER_LIM}
X_UPPER_LIM = {X_UPPER_LIM}
""".format(**values)

    with open("tmp_conf_file.ini","w") as f:
        f.writelines(tmp_ini_file_string)
    

def record_results(METHOD_SHORTNAME,SOLVER_POPSIZE,PROBLEM_INDEX,PROBLEM_DIM,MAX_SOLVER_FE,X_LOWER_LIM,X_UPPER_LIM,F_COMPETITION):
    write_conf_file(METHOD_SHORTNAME,SOLVER_POPSIZE,PROBLEM_INDEX,PROBLEM_DIM,MAX_SOLVER_FE,X_LOWER_LIM,X_UPPER_LIM,F_COMPETITION)
#    subprocess.run("./neat tmp_conf_file.ini > /dev/null",shell=True)
    subprocess.run("./neat tmp_conf_file.ini",shell=True)
    with open("result.txt","r") as f:
        res = f.readline().strip()
    print("["+str(F_COMPETITION)+","+res+"]"), # file="coparison_result.txt")
    subprocess.run("rm result.txt",shell=True)


        


df = pd.read_csv("src/experiments/real/results/comparison_other_pso/comparison_other_pso_methods.csv")


METHOD_SHORTNAME = ""
for row_index, row in df.iterrows():
    
    
    if str(row[0]).count("|") == 1: # new method
        problem_lims_row_index = row_index
        METHOD_SHORTNAME = row[0].split("|")[-1]
    
    elif str(row[0]).count("|") == 3: # new params, execute algorithm
        PROBLEM_DIM = int(row[0].split("|")[0])
        MAX_SOLVER_FE = int(row[0].split("|")[1])
        SOLVER_POPSIZE = int(row[0].split("|")[2])
        for col_index in range(1,len(row)):
            PROBLEM_INDEX = col_index # first problem is in col 1, and the problem index is 1 too.
            if  df.isnull().iloc[(problem_lims_row_index,PROBLEM_INDEX)]:
                continue
            X_LOWER_LIM = float(df.iloc[(problem_lims_row_index,PROBLEM_INDEX)].split("|")[0])
            X_UPPER_LIM = float(df.iloc[(problem_lims_row_index,PROBLEM_INDEX)].split("|")[1])
            F_COMPETITION = float(df.iloc[(row_index,PROBLEM_INDEX)])

            record_results(METHOD_SHORTNAME,SOLVER_POPSIZE,PROBLEM_INDEX,PROBLEM_DIM,MAX_SOLVER_FE,X_LOWER_LIM,X_UPPER_LIM,F_COMPETITION)
    

        

