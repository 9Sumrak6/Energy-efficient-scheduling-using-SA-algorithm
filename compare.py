# import matplotlib as plt


def plot(x, y):
    pass

if __name__ == "__main__":
    x = {}
    y_abs = {}
    y_opt = {}
    y_res = {}

    job_num = [10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 2000, 3000, 4000]
    proc_num = [2, 3, 4, 5, 10, 20, 40, 60, 80, 100, 120, 140, 160]

    for p_n in proc_num:
        x[p_n] = []
        y_abs[p_n] = []
        y_opt[p_n] = []
        y_res[p_n] = []

        for j_n in job_num:
            if p_n > j_n / 2:
                continue

            file_name = str(j_n) + "_" + str(p_n)

            path_opt = "Input/opt/" + file_name + ".txt"
            path_res = "Input/res/" + file_name + ".txt"

            f_opt = open(path_opt, "r")
            f_res = open(path_res, "r")

            abs_opt = f_opt.readline()
            opt = f_opt.readline()
            res = f_res.readline()

            x[p_n].append(file_name)
            y_abs[p_n].append(float(abs_opt))
            y_opt[p_n].append(float(opt))
            y_res[p_n].append(float(res))

            f_opt.close()
            f_res.close()

    print(x)
    print()
    print(y_abs)
    print()
    print(y_opt)
    print()
    print(y_res)
    print()
