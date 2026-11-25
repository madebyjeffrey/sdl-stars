include "S03-player-labels.dfy"
include "M04-page-initialization.dfy"

module M04TextProducers {
  import S = S01
  import L = S03PlayerLabels
  import F = S04IntegerFormat
  import P = M04PageCommands
  import I = M04PageInitialization
  import U = S04Utf8Prefix
  datatype TitleResult = TitleText(text: S.Bytes,id: S.U16) | CatalogBoundary(id: S.U16)
    | FormatBoundary | UndefinedNative
  function Title(readOnly: S.I16,step: S.I16): TitleResult {
    var id:=I.TitleId(270,readOnly);
    var row:=L.Catalog(id);
    if row.Absent? then CatalogBoundary(id) else
    if |row.bytes|>=50 then UndefinedNative else
    var formatted:=F.Format(row.bytes,step);
    if !formatted.Formatted? then FormatBoundary else
    if !F.Fits(formatted,360) then UndefinedNative else TitleText(formatted.text,id)
  }
  function AutoPlural(r: S.Bytes): L.Status
    requires |r|==192
  { L.Name(L.Names(r[128..160],r[160..192]),0,1,1,0,0) }
  predicate PluralReceipt(r: S.Bytes,supplied: S.Bytes)
    requires |r|==192
  {
    var result:=AutoPlural(r);
    result.Label? && |result.text|<32 && supplied==result.text
  }
  // This is the producer called after the new singular is copied and plural
  // zeroed. The command model's returned plural is refined, not redefined.
  function DefaultPlural(index: nat): L.Status
    requires index<7
  { L.Name(L.Names(L.Field(L.Defaults()[index]),L.Field([])),0,1,1,0,0) }
  predicate CurrentDefaultReceipt(x: P.Input) {
    x.defaultNames==L.Defaults() &&
    (271<=x.id<=277 && x.notification==0 && P.MatchName(x.name,x.defaultNames)<7 ==>
      DefaultPlural(x.id-271).Label? && x.autoPlural==DefaultPlural(x.id-271).text)
  }
  function Prefix(readOnly: S.I16): S.Bytes {
    if readOnly==0 then L.B("Custom Race Wizard - Step ") else L.B("View Race - Page ")
  }
  lemma {:isolate_assertions} OrdinaryTitle(readOnly: S.I16,step: S.I16)
    requires readOnly==0 || readOnly==1
    ensures Title(readOnly,step).TitleText?
    ensures Title(readOnly,step).id==270+readOnly
    ensures Title(readOnly,step).text==
      Prefix(readOnly)+F.Decimal(step)+L.B(" of 6")
  {
    var prefix:=if readOnly==0 then "Custom Race Wizard - Step " else "View Race - Page ";
    L.BConcat("Custom ","Race ");
    L.BConcat("Custom Race ","Wizard ");
    L.BConcat("Custom Race Wizard ","- Step ");
    L.BConcat("View ","Race ");
    L.BConcat("View Race ","- Page ");
    assert U.ByteString(prefix);
    L.BConcat(prefix,"%d");
    L.BConcat(prefix+"%d"," of 6");
    assert L.Catalog(270+readOnly).bytes==L.B(prefix)+[37,100]+L.B(" of 6");
    L.Excludes(prefix,0);
    L.Excludes(prefix,37);
    L.Excludes(" of 6",0);
    L.Excludes(" of 6",37);
    F.DecimalTemplate(L.B(prefix),L.B(" of 6"),step);
    F.DecimalI16(step);
  }
  lemma UnknownTitle()
    ensures Title(-32768,1)==CatalogBoundary(33038)
  {
    assert I.TitleId(270,-32768)==33038;
    assert L.Catalog(33038)==L.Absent;
  }
  lemma NoNameTitle()
    ensures Title(1099,6)==TitleText(L.B("No Name"),1369)
  {
    assert I.TitleId(270,1099)==1369;
    assert L.Catalog(1369)==L.Text(L.B("No Name"));
    F.NoPlaceholderConsumesNothing(L.B("No Name"),6);
  }
  lemma {:isolate_assertions} TitleVectors() {
    UnknownTitle();
    NoNameTitle();
    F.WideDecimalVectors();
    OrdinaryTitle(0,1);
    OrdinaryTitle(1,6);
    OrdinaryTitle(1,-32768);
    F.NoPlaceholderConsumesNothing(L.B("No Name"),6);
    L.AutomaticPlain(L.B("Humanoid"));
    L.AutomaticPlain(L.B("Random"));
    L.BConcat("Humanoid","s");
    L.BConcat("Random","s");
    L.BConcat("Custom Race Wizard - Step ","1");
    L.BConcat("Custom Race Wizard - Step 1"," of 6");
    L.BConcat("View Race - Page ","6");
    L.BConcat("View Race - Page 6"," of 6");
    L.BConcat("View Race - Page ","-32768");
    L.BConcat("View Race - Page -32768"," of 6");
    assert Title(0,1)==TitleText(L.B("Custom Race Wizard - Step 1 of 6"),270);
    assert Title(1,6)==TitleText(L.B("View Race - Page 6 of 6"),271);
    assert Title(1,-32768)==TitleText(L.B("View Race - Page -32768 of 6"),271);
    assert Title(1099,6)==TitleText(L.B("No Name"),1369);
    assert Title(-32768,1)==CatalogBoundary(33038);
    assert DefaultPlural(0)==L.Label(L.B("Humanoids"),[]);
    assert DefaultPlural(6)==L.Label(L.B("Randoms"),[]);
  }
}
