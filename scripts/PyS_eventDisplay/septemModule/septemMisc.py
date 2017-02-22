# functions not related to files, plotting or classes

def add_line_to_header(header, str_to_add, value):
    # adds an additional line to a header of a plot
    string = ("\n%s : " % str_to_add).ljust(25)
    string += str(len(self.ns.eventSet))
    header += string
    return header

    
