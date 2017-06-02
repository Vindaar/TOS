#!/usr/bin/env julia

using PyCall
@pyimport matplotlib as mpl
mpl.use("TKAgg")
mpl.interactive(false)

using PyPlot

function get_frame_from_file(path, file)
    filename = string(path, file)
    print("Reading ", filename)
    try
        frame = readdlm(filename)
        return frame
    catch e
        if isa(e, LoadError) | isa(e, SystemError)
            print("File ", filename, " not found.")
            return zeros(256,256)
        else
            rethrow()
        end
    end
end

function poll_new_file(fm)

    # while true
    #     file_mod = wait(fm::FileMonitor)
    #     if 
    #         file = file_mod[1]
    #     end
    # end
    path = "/home/schmidt/TOS/tmp/framedumps/"
    file_mod = wait(fm::FileMonitor)
    sleep(0.07)
    print(file_mod)
    file = file_mod[1]
    # s = string(path, file)
    # fn = open(s)
    # close(fn)

    return file
end

function draw_new_frame(fig, cb, image, frame)
    println("Drawing new frame")
    image[:set_data](frame)
    image[:autoscale]()
    #image[:set_clim](0, 1)
    cb[:update_normal](image)
    fig[:canvas][:draw]()
    draw()
end

function poll_and_get_frame(fm, path)
    file = poll_new_file(fm)
    print("Working on ", file)
    frame = get_frame_from_file(path, file)

    return frame
end

function try_to_draw(fig, cb, image, frame)
    try
        draw_new_frame(fig, cb, image, frame)
    catch e
        if isa(e, LoadError)
            draw_new_frame(fig, cb, image, frame)
        end
    end
end

function main(interactive)

    path1 = "/home/schmidt/TOS/tmp/framedumps/"
    path2 = "/home/schmidt/TOS/data/singleFrames/"

    files = readdir(path1)
    
    #f, ax = subplots()
    fig = plt[:figure]()
    subplot = fig[:add_subplot](111)

    if length(files) > 0
        files = sort(files)
        frame = get_frame_from_file(path1, files[1])
        image = subplot[:imshow](frame, interpolation="none", axes=subplot)
    else
        image = subplot[:imshow](zeros(256,256), interpolation="none", axes=subplot)
    end


    #plt[:draw]()
    plt[:show](block=false)
    cbaxes = fig[:add_axes]([0.85, 0.1, 0.03, 0.7])
    cb     = plt[:colorbar](image, cax = cbaxes)

    # @spawn fm1 = FileMonitor(path1)
    # @spawn fm2 = FileMonitor(path2)
    fm1 = FileMonitor(path1)
    fm2 = FileMonitor(path2)
    #for file in files[2:end]

    ch = Channel{Any}(1)

    @sync begin
        @async while true
            put!(ch, poll_and_get_frame(fm1, path1))
            frame = take!(ch)
            try_to_draw(fig, cb, image, frame)
        end
        @async while true
            put!(ch, poll_and_get_frame(fm2, path2))
            frame = take!(ch)
            try_to_draw(fig, cb, image, frame)
        end
    end
    
  
    
    # while true
    #     # file = fetch(@spawn(poll_new_file(fm1)))
    #     # file = fetch(@spawn(poll_new_file(fm2)))
    #     #file = wait(@async(poll_new_file(fm1)))
    #     # file = wait(@async(poll_new_file(fm2)))
    #     # print("Working on ", file)
    #     # frame = get_frame_from_file(path2, file)
    #     # @async begin
    #     #     @sync begin 
    #     #         frame = poll_and_get_frame(fm1, path1)
    #     #         try_to_draw(fig, cb, image, frame)
    #     #     end
    #     #     @sync begin
    #     #         frame = poll_and_get_frame(fm2, path2)
    #     #         try_to_draw(fig, cb, image, frame)
    #     #     end
    #     # end
    #     #@sync()
    #     @async begin
    #         @spawn(put!(ch, poll_and_get_frame(fm1, path1)))
    #         @spawn(put!(ch, poll_and_get_frame(fm2, path2)))
    #     end
    #     frame = take!(ch)
    #     try_to_draw(fig, cb, image, frame)
        
    #     #draw_new_frame(fig, image, frame)
    # end

    #show()

end 

interactive = false
main(interactive)
