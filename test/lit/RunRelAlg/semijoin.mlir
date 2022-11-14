//RUN: mlir-db-opt -lower-relalg-to-subop %s | run-mlir "-" %S/../../../resources/data/uni | FileCheck %s
//CHECK: |                        s.name  |
//CHECK: ----------------------------------
//CHECK: |                       "Jonas"  |
//CHECK: |                      "Fichte"  |
//CHECK: |                "Schopenhauer"  |
//CHECK: |                      "Carnap"  |
//CHECK: |                "Theophrastos"  |
//CHECK: |                   "Feuerbach"  |


module @querymodule{
    func.func @main () {
        %1 = relalg.basetable { table_identifier="hoeren" } columns: {matrnr => @hoeren::@matrnr({type=i64}),
            vorlnr => @hoeren::@vorlnr({type=i64})
        }
        %2 = relalg.basetable { table_identifier="studenten" } columns: {matrnr => @studenten::@matrnr({type=i64}),
            name => @studenten::@name({type=!db.string}),
            semester => @studenten::@semester({type=i64})
        }
        %3 = relalg.semijoin %2, %1 (%6: !tuples.tuple) {
                                                 %8 = tuples.getcol %6 @hoeren::@matrnr : i64
                                                 %9 = tuples.getcol %6 @studenten::@matrnr : i64
                                                 %10 = db.compare eq %8 : i64,%9 : i64
                                                 tuples.return %10 : i1
                                             }

        %15 = relalg.materialize %3 [@studenten::@name] => ["s.name"] : !subop.result_table<[sname: !db.string]>
        subop.set_result 0 %15 : !subop.result_table<[sname: !db.string]>
        return
    }
}