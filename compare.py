import matplotlib.pyplot as plt
import numpy as np


def read_energy(path_name, opt):
    try:
        data = open(path_name, 'r')
        res = data.readline().split('=')[1]
        data.close()
    except:
        return opt

    return float(res)


def read_time(path_name, alt = 0):
    try:
        data = open(path_name, 'r')
        res = data.readline()
        res = data.readline().split('=')[1]
        data.close()
    except:
        return alt

    if res[-2] == 's':
        return float(res[:-3]) / 1000000
    return float(res) / 1000000


def read_iterations(path_name, opt):
    try:
        data = open(path_name, 'r')
        res = data.readline()
        res = data.readline()
        res = data.readline().split('=')[1]
        data.close()
    except:
        return 1e12

    return float(res)


if __name__ == "__main__":
    #--------ENERGY--------
    # Plot proc=const jobs - variable
    x = {}
    y_opt_energy = {}
    y_fork_energy = {}
    y_mpi_energy = {}
    y_cons_energy = {}

    job_num = ["10", "20", "30", "40", "50", "100", "200", "300", "400", "500", "1000", "2000", "3000", "4000"]
    proc_num = ["2", "3", "4", "5", "10", "20", "40", "60", "80", "100", "120", "140", "160"]

    for law in ['boltz/', 'cauchy/', 'common/']:
        x[law] = {}
        y_opt_energy[law] = {}
        y_fork_energy[law] = {}
        y_mpi_energy[law] = {}
        y_cons_energy[law] = {}

        for p_n in proc_num:
            x[law][p_n] = []
            y_opt_energy[law][p_n] = []
            y_fork_energy[law][p_n] = []
            y_mpi_energy[law][p_n] = []
            y_cons_energy[law][p_n] = []

            for j_n in job_num:
                if int(p_n) > int(j_n) / 2:
                    continue

                file_name = j_n + "_" + p_n

                path_opt = "Input/opt/" + file_name + ".txt"
                path_fork = "Output/fork_mpi/10/" + law + file_name + ".txt"
                path_fork1 = "Output1/fork_mpi/10/" + law + file_name + ".txt"
                path_mpi = "Output/mpi/10/" + law + file_name + ".txt"
                path_mpi1 = "Output/mpi/10/" + law + file_name + ".txt"
                path_cons = "Output/consecutive/" + law + file_name + ".txt"

                f_opt = open(path_opt, "r")
                abs_opt = f_opt.readline()
                opt = float(f_opt.readline())

                fork = read_energy(path_fork, opt)
                fork1 = read_energy(path_fork1, opt)
                mpi = read_energy(path_mpi, opt)
                mpi1 = read_energy(path_mpi1, opt)
                cons = read_energy(path_cons, opt)

                x[law][p_n].append(j_n)
                y_opt_energy[law][p_n].append(opt)
                y_fork_energy[law][p_n].append(min(fork, fork1))
                y_mpi_energy[law][p_n].append(min(mpi, mpi1))
                y_cons_energy[law][p_n].append(float(cons))

                f_opt.close()


    font = {'weight' : 'bold', 'size' : 16}
    for law in ['boltz/', 'cauchy/', 'common/']:
        for i in x[law]:
            plt.rc('font', **font)
            X = x[law][i]
            opt = np.array(y_opt_energy[law][i], dtype=float)
            fork = np.array(y_fork_energy[law][i], dtype=float)
            mpi = np.array(y_mpi_energy[law][i], dtype=float)
            cons = np.array(y_cons_energy[law][i], dtype=float)

            plt.figure(figsize=(20, 10))
            plt.grid(True)
            plt.title(f'{i} processors')

            plt.plot(X, fork / opt, label='fork')
            plt.plot(X, mpi / opt, label='mpi')
            plt.plot(X, cons / opt, label='cons')

            plt.ylabel('E_res / E_best')
            plt.xlabel(f'Number of jobs')

            plt.legend()
            plt.savefig(f'Graphics/energy/jobs/{law}{i}.png', bbox_inches='tight')
            plt.close()


    # Plot job=const procs - variable
    x = {}
    y_opt_energy = {}
    y_fork_energy = {}
    y_mpi_energy = {}
    y_cons_energy = {}

    for law in ['boltz/', 'cauchy/', 'common/']:
        x[law] = {}
        y_opt_energy[law] = {}
        y_fork_energy[law] = {}
        y_mpi_energy[law] = {}
        y_cons_energy[law] = {}

        for j_n in job_num:
            x[law][j_n] = []
            y_opt_energy[law][j_n] = []
            y_fork_energy[law][j_n] = []
            y_mpi_energy[law][j_n] = []
            y_cons_energy[law][j_n] = []

            for p_n in proc_num:
                if int(p_n) > int(j_n) / 2:
                    continue

                file_name = j_n + "_" + p_n

                path_opt = "Input/opt/" + file_name + ".txt"
                path_fork = "Output/fork_mpi/10/" + law + file_name + ".txt"
                path_fork1 = "Output1/fork_mpi/10/" + law + file_name + ".txt"
                path_mpi = "Output/mpi/10/" + law + file_name + ".txt"
                path_mpi1 = "Output/mpi/10/" + law + file_name + ".txt"
                path_cons = "Output/consecutive/" + law + file_name + ".txt"

                f_opt = open(path_opt, "r")
                abs_opt = f_opt.readline()
                opt = float(f_opt.readline())

                fork = read_energy(path_fork, opt)
                fork1 = read_energy(path_fork1, opt)
                mpi = read_energy(path_mpi, opt)
                mpi1 = read_energy(path_mpi1, opt)
                cons = read_energy(path_cons, opt)

                x[law][j_n].append(p_n)
                y_opt_energy[law][j_n].append(opt)
                y_fork_energy[law][j_n].append(min(fork, fork1))
                y_mpi_energy[law][j_n].append(min(mpi, mpi1))
                y_cons_energy[law][j_n].append(float(cons))

                f_opt.close()


    font = {'weight' : 'bold', 'size' : 16}
    for law in ['boltz/', 'cauchy/', 'common/']:
        for i in x[law]:
            plt.rc('font', **font)
            X = x[law][i]
            opt = np.array(y_opt_energy[law][i], dtype=float)
            fork = np.array(y_fork_energy[law][i], dtype=float)
            mpi = np.array(y_mpi_energy[law][i], dtype=float)
            cons = np.array(y_cons_energy[law][i], dtype=float)

            plt.figure(figsize=(20, 10))
            plt.grid(True)
            plt.title(f'{i} jobs')

            plt.plot(X, fork / opt, label='fork')
            plt.plot(X, mpi / opt, label='mpi')
            plt.plot(X, cons / opt, label='cons')

            plt.ylabel('E_res / E_best')
            plt.xlabel(f'Number of processors')

            plt.legend()
            plt.savefig(f'Graphics/energy/procs/{law}{i}.png', bbox_inches='tight')
            plt.close()
    
    #--------TIME--------
    # Plot proc=const jobs - variable
    x = {}
    y_opt_time = {}
    y_fork_time = {}
    y_mpi_time = {}
    y_cons_time = {}

    for law in ['boltz/', 'cauchy/', 'common/']:
        x[law] = {}
        y_opt_time[law] = {}
        y_fork_time[law] = {}
        y_mpi_time[law] = {}
        y_cons_time[law] = {}

        for p_n in proc_num:
            x[law][p_n] = []
            y_opt_time[law][p_n] = []
            y_fork_time[law][p_n] = []
            y_mpi_time[law][p_n] = []
            y_cons_time[law][p_n] = []

            for j_n in job_num:
                if int(p_n) > int(j_n) / 2:
                    continue

                file_name = j_n + "_" + p_n

                path_opt = "Input/opt/" + file_name + ".txt"
                path_fork = "Output/fork_mpi/10/" + law + file_name + ".txt"
                path_fork1 = "Output1/fork_mpi/10/" + law + file_name + ".txt"
                path_mpi = "Output/mpi/10/" + law + file_name + ".txt"
                path_mpi1 = "Output/mpi/10/" + law + file_name + ".txt"
                path_cons = "Output/consecutive/" + law + file_name + ".txt"

                fork = read_time(path_fork)
                fork1 = read_time(path_fork1, fork)
                mpi = read_time(path_mpi, fork)
                mpi1 = read_time(path_mpi1, fork)
                cons = read_time(path_cons)

                x[law][p_n].append(j_n)
                y_fork_time[law][p_n].append(min(fork, fork1))
                y_mpi_time[law][p_n].append(min(mpi, mpi1))
                y_cons_time[law][p_n].append(float(cons))

                f_opt.close()


    font = {'weight' : 'bold', 'size' : 16}
    for law in ['boltz/', 'cauchy/', 'common/']:
        for i in x[law]:
            plt.rc('font', **font)
            X = x[law][i]
            opt = np.array(y_opt_time[law][i], dtype=float)
            fork = np.array(y_fork_time[law][i], dtype=float)
            mpi = np.array(y_mpi_time[law][i], dtype=float)
            cons = np.array(y_cons_time[law][i], dtype=float)

            plt.figure(figsize=(20, 10))
            plt.grid(True)
            plt.title(f'{i} processors')

            plt.plot(X, fork, label='fork')
            plt.plot(X, mpi, label='mpi')
            plt.plot(X, cons, label='cons')

            plt.ylabel('Time (s)')
            plt.xlabel('Number of jobs')

            plt.legend()
            plt.savefig(f'Graphics/time/jobs/{law}{i}.png', bbox_inches='tight')
            plt.close()
    
     # Plot job=const procs - variable
    x = {}
    y_opt_time = {}
    y_fork_time = {}
    y_mpi_time = {}
    y_cons_time = {}

    for law in ['boltz/', 'cauchy/', 'common/']:
        x[law] = {}
        y_opt_time[law] = {}
        y_fork_time[law] = {}
        y_mpi_time[law] = {}
        y_cons_time[law] = {}

        for j_n in job_num:
            x[law][j_n] = []
            y_opt_time[law][j_n] = []
            y_fork_time[law][j_n] = []
            y_mpi_time[law][j_n] = []
            y_cons_time[law][j_n] = []

            for p_n in proc_num:
                if int(p_n) > int(j_n) / 2:
                    continue

                file_name = j_n + "_" + p_n

                path_opt = "Input/opt/" + file_name + ".txt"
                path_fork = "Output/fork_mpi/10/" + law + file_name + ".txt"
                path_fork1 = "Output1/fork_mpi/10/" + law + file_name + ".txt"
                path_mpi = "Output/mpi/10/" + law + file_name + ".txt"
                path_mpi1 = "Output/mpi/10/" + law + file_name + ".txt"
                path_cons = "Output/consecutive/" + law + file_name + ".txt"

                f_opt = open(path_opt, "r")
                abs_opt = f_opt.readline()
                opt = float(f_opt.readline())

                fork = read_time(path_fork)
                fork1 = read_time(path_fork1, fork)
                mpi = read_time(path_mpi, fork)
                mpi1 = read_time(path_mpi1, fork)
                cons = read_time(path_cons)

                x[law][j_n].append(p_n)
                y_opt_time[law][j_n].append(opt)
                y_fork_time[law][j_n].append(min(fork, fork1))
                y_mpi_time[law][j_n].append(min(mpi, mpi1))
                y_cons_time[law][j_n].append(float(cons))

                f_opt.close()


    font = {'weight' : 'bold', 'size' : 16}
    for law in ['boltz/', 'cauchy/', 'common/']:
        for i in x[law]:
            plt.rc('font', **font)
            X = x[law][i]
            opt = np.array(y_opt_time[law][i], dtype=float)
            fork = np.array(y_fork_time[law][i], dtype=float)
            mpi = np.array(y_mpi_time[law][i], dtype=float)
            cons = np.array(y_cons_time[law][i], dtype=float)

            plt.figure(figsize=(20, 10))
            plt.grid(True)
            plt.title(f'{i} jobs')

            plt.plot(X, fork, label='fork')
            plt.plot(X, mpi, label='mpi')
            plt.plot(X, cons, label='cons')

            plt.ylabel('Time (s)')
            plt.xlabel(f'Number of processors')

            plt.legend()
            plt.savefig(f'Graphics/time/procs/{law}{i}.png', bbox_inches='tight')
            plt.close()