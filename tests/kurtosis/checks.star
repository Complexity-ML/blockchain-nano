suite = import_module("./main.star")
def run(plan, args):
    suite.check_finality(plan)
