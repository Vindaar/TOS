#!/usr/bin/env julia

using PyCall
pygui(:tk)
@pyimport matplotlib as mpl
mpl.use("TKAgg")
mpl.interactive(true)
@pyimport matplotlib.dates as mpl_dates

using PyPlot


function read_temp_file(path, filename)
    filepath = string(path, filename)
    columns  = readlines(filepath)#readdlm(filepath, skipstart=2)
    #columns  = [el for el in columns if contains(el, "#") == false]
    imb = Float64[]
    dates = AbstractString[]
    for el in columns
        if contains(el, "#") == false
            el = split(el)
            t::Float64 = parse(Float64, el[1])
            #println(el[1])
            d::AbstractString = el[3]
            #println(d)
            if t > 0
                push!(imb, Float64(t))
                push!(dates, d)
            end
        end
    end
    
    
    #imb      = columns[:,1]
    #dates    = columns[:,3]
    println(imb[1])
    println(dates[1])

    date_syntax = get_TOS_date_syntax()
    date_format = Dates.DateFormat(date_syntax)
    dates = [DateTime(d, date_format) for d in dates]
    
    return imb, dates
end

function get_TOS_date_syntax()
    # return date time syntax of temp logs from TOS
    val = "y-m-d.H:M:S"
    return val
end


function get_temp_filename()
    return "temp_log.txt"
end

function get_second_temp()
    return "/home/schmidt/log/temp_log_29_05_17.txt"
end

function plot_temps(fig, ax, imb1, dates1)

    ax[:clear]()
    ax[:set_ylabel]("Temp / °C")
    date_day = Date(dates1[1])
    date_day = Dates.format(date_day, "dd-mm-yyyy")
    println(date_day)
    title = string("Temperature in office 2.031 @ ", date_day)
    plt[:title](title)

    filename = get_temp_filename()
    path = "/home/schmidt/TOS/log/"
    
    imb2, dates2 = read_temp_file(path, filename)
    
    imb = vcat(imb1, imb2)
    dates = vcat(dates1, dates2)

    println(length(imb))
    println(length(dates))

    datemin = minimum(dates)
    datemax = maximum(dates)
    
    ax[:set_xlim](datemin, datemax)
    ax[:xaxis][:set_major_formatter](mpl_dates.DateFormatter("%H-%M-%S"))
    ax[:xaxis][:set_major_locator](mpl_dates.HourLocator())    
    ax[:plot_date](dates, imb, label=filename, markersize = 1)
    plt[:show](block=false)
    #draw()
end

function main()
    second_filename = get_second_temp()
    imb1, dates1 = read_temp_file("", second_filename)
    fig, ax = plt[:subplots]()
    
    while true
        plot_temps(fig, ax, imb1, dates1)
        sleep(10)
    end

    show()
end

main()
