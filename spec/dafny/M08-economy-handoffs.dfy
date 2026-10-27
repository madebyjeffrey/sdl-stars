include "M19-native-growth.dfy"
include "M09-forecast.dfy"
include "M09-host-production.dfy"

// Connect native growth/resource producers to their existing numeric callers.
// Query-player operation rates and owner-player growth remain separate inputs.
module M08EconomyHandoffs {
  import E = M08NativeEconomy
  import G = M19NativeGrowth
  import S = S01
  import F = M09Forecast
  import H = M09HostProduction
  import K = M09Costs
  function GrowthValue(result: G.Result): E.Number {
    if result.Computed? then E.Value(result.change) else E.Undefined(result.reason)
  }
  function NextIndustry(absolute: E.Number,pop: E.I32,delta: S.U8,operate: E.I8,next: bool,operationPrimary: E.I8,
    owned: bool,hab: S.I16,ownerCapacity: E.Number,ownerPrimary: E.I8,ideal: E.I8): E.Number
    requires absolute.Value? ==> -32768<=absolute.value<32768
  {
    if absolute.Undefined? then absolute else
    var growth:=if !next then E.Undefined("uncalled") else GrowthValue(G.Grow(owned,pop,delta,hab,ownerCapacity,ownerPrimary,ideal,false));
    E.Operable(absolute,pop,operate,next,growth,operationPrimary)
  }
  datatype Budget = NoReturn(reason: string) | ForecastBudget(forecast: F.Budget)
    | HostBudget(host: H.Budget) | HostZero
  function ForecastResources(resources: E.Number,pct: E.I8,suppressed: bool): Budget
    requires resources.Value? ==> -32768<=resources.value<32768
  {
    if resources.Undefined? then NoReturn(resources.reason) else
    ForecastBudget(F.ReserveResources(resources.value as S.I16,pct as K.I8,suppressed))
  }
  function HostQueueResources(resources: E.Number,extra: S.U16,pct: E.I8,suppressed: bool,annual: E.I32): Budget
    requires resources.Value? ==> -32768<=resources.value<32768
  {
    if resources.Undefined? then NoReturn(resources.reason) else
    var adjusted:=H.Extra(resources.value as S.I16,extra);
    if adjusted.Undefined? then NoReturn(adjusted.reason) else
    if adjusted.value==0 then HostZero else
    var reserved:=H.Reserve(adjusted.value,pct,suppressed,annual);
    if reserved.UndefinedBudget? then NoReturn(reserved.reason) else HostBudget(reserved)
  }
  lemma NoUncalledGrowth(absolute: E.Number,pop: E.I32,delta: S.U8,operate: E.I8,operationPrimary: E.I8,
    owned: bool,hab: S.I16,ownerCapacity: E.Number,ownerPrimary: E.I8,ideal: E.I8)
    requires absolute.Value? ==> -32768<=absolute.value<32768
    ensures NextIndustry(absolute,pop,delta,operate,false,operationPrimary,owned,hab,ownerCapacity,ownerPrimary,ideal)==
      E.Operable(absolute,pop,operate,false,E.Undefined("uncalled"),operationPrimary)
  {}
}
