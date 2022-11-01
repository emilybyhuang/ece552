import sys
import os
from genericpath import isdir, isfile

TestList = [
  "astar",
  "bwaves",
  "bzip2",
  "gcc",
  "gromacs",
  "hmmer",
  "mcf",
  "soplex"
]

# ../cbp4-assign2/predictor /cad2/ece552f/cbp4_benchmarks/astar.cbp4.gz

TracePath = "/cad2/ece552f/cbp4_benchmarks/{}.cbp4.gz"
PredictorPath = "../cbp4-assign2/predictor"
# PredictorPath = "../cbp4-assign2"

def ErrLog(str):
  str = "<Error> " + str
  print(str)
  exit(-1)

if __name__ == "__main__":
  if isfile(PredictorPath) == False:
    ErrLog("Predictor Not found. Please check the predictor path.")

  for Test in TestList:
    print("Testing: {}".format(Test))
    CurrTrace = TracePath.format(Test)
    FileDump = Test + ".out"
    os.system("{} {} | tee {}".format(PredictorPath, CurrTrace, FileDump))

  TestResult = []

  for Test in TestList:
    FileDump = Test + ".out"
    if isfile(FileDump) == False:
      print("Test output not found.")
      exit(-1)
    F = open(FileDump, "r")
    for Line in F:
      if "openend" in Line and "MISPRED_PER_1K_INST" in Line:
        Tokens = Line.split(":")
        Num = Tokens[2].lstrip(' ')
        TestResult.append(float(Num))
        break
  
  Sum = 0
  for idx in range(len(TestList)):
    print("{}: {}".format(TestList[idx], str(TestResult[idx])))
    Sum += TestResult[idx]
  
  Ave = Sum / len(TestList)
  print("Average: {}".format(Ave))
        
