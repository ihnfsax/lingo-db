module @querymodule{
    func  @main ()  -> !db.table{
        %1 = relalg.basetable @orders { table_identifier="orders", rows=150000 , pkey=["o_orderkey"]} columns: {o_orderkey => @o_orderkey({type=i32}),
            o_custkey => @o_custkey({type=i32}),
            o_orderstatus => @o_orderstatus({type=!db.char<1>}),
            o_totalprice => @o_totalprice({type=!db.decimal<15,2>}),
            o_orderdate => @o_orderdate({type=!db.date<day>}),
            o_orderpriority => @o_orderpriority({type=!db.string}),
            o_clerk => @o_clerk({type=!db.string}),
            o_shippriority => @o_shippriority({type=i32}),
            o_comment => @o_comment({type=!db.string})
        }
        %2 = relalg.basetable @lineitem { table_identifier="lineitem", rows=600572 , pkey=["l_orderkey","l_linenumber"]} columns: {l_orderkey => @l_orderkey({type=i32}),
            l_partkey => @l_partkey({type=i32}),
            l_suppkey => @l_suppkey({type=i32}),
            l_linenumber => @l_linenumber({type=i32}),
            l_quantity => @l_quantity({type=!db.decimal<15,2>}),
            l_extendedprice => @l_extendedprice({type=!db.decimal<15,2>}),
            l_discount => @l_discount({type=!db.decimal<15,2>}),
            l_tax => @l_tax({type=!db.decimal<15,2>}),
            l_returnflag => @l_returnflag({type=!db.char<1>}),
            l_linestatus => @l_linestatus({type=!db.char<1>}),
            l_shipdate => @l_shipdate({type=!db.date<day>}),
            l_commitdate => @l_commitdate({type=!db.date<day>}),
            l_receiptdate => @l_receiptdate({type=!db.date<day>}),
            l_shipinstruct => @l_shipinstruct({type=!db.string}),
            l_shipmode => @l_shipmode({type=!db.string}),
            l_comment => @l_comment({type=!db.string})
        }
        %3 = relalg.crossproduct %1, %2
        %5 = relalg.selection %3(%4: !relalg.tuple) {
            %6 = relalg.getattr %4 @orders::@o_orderkey : i32
            %7 = relalg.getattr %4 @lineitem::@l_orderkey : i32
            %8 = db.compare eq %6 : i32,%7 : i32
            %9 = relalg.getattr %4 @lineitem::@l_shipmode : !db.string
            %10 = db.constant ("MAIL") :!db.string
            %11 = db.compare eq %9 : !db.string,%10 : !db.string
            %12 = db.constant ("SHIP") :!db.string
            %13 = db.compare eq %9 : !db.string,%12 : !db.string
            %14 = db.or %11 : i1,%13 : i1
            %15 = relalg.getattr %4 @lineitem::@l_commitdate : !db.date<day>
            %16 = relalg.getattr %4 @lineitem::@l_receiptdate : !db.date<day>
            %17 = db.compare lt %15 : !db.date<day>,%16 : !db.date<day>
            %18 = relalg.getattr %4 @lineitem::@l_shipdate : !db.date<day>
            %19 = relalg.getattr %4 @lineitem::@l_commitdate : !db.date<day>
            %20 = db.compare lt %18 : !db.date<day>,%19 : !db.date<day>
            %21 = relalg.getattr %4 @lineitem::@l_receiptdate : !db.date<day>
            %22 = db.constant ("1994-01-01") :!db.date<day>
            %23 = db.compare gte %21 : !db.date<day>,%22 : !db.date<day>
            %24 = relalg.getattr %4 @lineitem::@l_receiptdate : !db.date<day>
            %25 = db.constant ("1995-01-01") :!db.date<day>
            %26 = db.compare lt %24 : !db.date<day>,%25 : !db.date<day>
            %27 = db.and %8 : i1,%14 : i1,%17 : i1,%20 : i1,%23 : i1,%26 : i1
            relalg.return %27 : i1
        }
        %29 = relalg.map @map %5 (%28: !relalg.tuple) {
            %30 = relalg.getattr %28 @orders::@o_orderpriority : !db.string
            %31 = db.constant ("1-URGENT") :!db.string
            %32 = db.compare eq %30 : !db.string,%31 : !db.string
            %33 = relalg.getattr %28 @orders::@o_orderpriority : !db.string
            %34 = db.constant ("2-HIGH") :!db.string
            %35 = db.compare eq %33 : !db.string,%34 : !db.string
            %36 = db.or %32 : i1,%35 : i1
            %37 = db.derive_truth %36 : i1
            %41 = scf.if %37  -> (i64) {
                %39 = db.constant (1) :i64
                scf.yield %39 : i64
            } else {
                %40 = db.constant (0) :i64
                scf.yield %40 : i64
            }
            %42 = relalg.addattr %28, @aggfmname1({type=i64}) %41
            %43 = relalg.getattr %28 @orders::@o_orderpriority : !db.string
            %44 = db.constant ("1-URGENT") :!db.string
            %45 = db.compare neq %43 : !db.string,%44 : !db.string
            %46 = relalg.getattr %28 @orders::@o_orderpriority : !db.string
            %47 = db.constant ("2-HIGH") :!db.string
            %48 = db.compare neq %46 : !db.string,%47 : !db.string
            %49 = db.and %45 : i1,%48 : i1
            %50 = db.derive_truth %49 : i1
            %54 = scf.if %50  -> (i64) {
                %52 = db.constant (1) :i64
                scf.yield %52 : i64
            } else {
                %53 = db.constant (0) :i64
                scf.yield %53 : i64
            }
            %55 = relalg.addattr %42, @aggfmname3({type=i64}) %54
            relalg.return %55 : !relalg.tuple
        }
        %58 = relalg.aggregation @aggr %29 [@lineitem::@l_shipmode] (%56 : !relalg.tuplestream, %57 : !relalg.tuple) {
            %59 = relalg.aggrfn sum @map::@aggfmname1 %56 : i64
            %60 = relalg.addattr %57, @aggfmname2({type=i64}) %59
            %61 = relalg.aggrfn sum @map::@aggfmname3 %56 : i64
            %62 = relalg.addattr %60, @aggfmname4({type=i64}) %61
            relalg.return %62 : !relalg.tuple
        }
        %63 = relalg.sort %58 [(@lineitem::@l_shipmode,asc)]
        %64 = relalg.materialize %63 [@lineitem::@l_shipmode,@aggr::@aggfmname2,@aggr::@aggfmname4] => ["l_shipmode","high_line_count","low_line_count"] : !db.table
        return %64 : !db.table
    }
}

