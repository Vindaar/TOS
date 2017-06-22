#!/usr/bin/env julia


function get_data_from_file(path, file)
    filename = string(path, file)
    columns = readdlm(filename)

    THL  = columns[:,1]
    vals = columns[:,2]

    return THL, vals
end

function main(chip)

    path = "/home/schmidt/TOS/data/singleFrames/chip_"
    path = string(path, chip, "/")
    files = readdir(path)
    for file in files
        THL, vals = get_data_from_file(path, file)
        plot(THL, vals, label=file)
    end
    plt[:title](path)
    legend()
    show()
end


using PyCall
@pyimport matplotlib as mpl
mpl.use("TKAgg")
mpl.interactive(true)

Pkg.add("PyPlot")
using PyPlot
for i in range(0, 7)
    main(i)
end
