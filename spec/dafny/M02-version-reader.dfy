include "M02-native-race-state.dfy"

module M02VersionReader {
  import S = S01
  import N = M02NativeRaceState
  // hf is left at the parser's resulting value; the wrapper restores only
  // its race snapshot, fFileErrSilent and wVersFile.
  datatype Context = Context(native: N.State,silent: S.I16,version: S.U16,handle: S.I16)
  datatype Inputs = Inputs(pathInfo: bool,directory: bool,link: bool,fileOpened: bool,bofCurrentRace: bool,
    bofCloseFailed: bool,parserResult: S.I16,decoded: Context)
  datatype Call = PathInfo | Open | ReadBof | CloseBof(failed: bool) | Capture | SetSilent
    | Parse(askPassword: bool) | CopyOutput | Restore | RestoreSilentVersion
  datatype Result = Result(success: bool,output: S.Bytes,context: Context,calls: seq<Call>)
  function Read(prior: Context,output: S.Bytes,i: Inputs): Result
    requires N.Shape(prior.native) && N.Shape(i.decoded.native) && |output|==192
  {
    if !i.pathInfo || i.directory || i.link then Result(false,output,prior,[PathInfo]) else
    if !i.fileOpened then Result(false,output,prior,[PathInfo,Open]) else
    var prefix:=[PathInfo,Open,ReadBof,CloseBof(i.bofCloseFailed)];
    if !i.bofCurrentRace then Result(false,output,prior,prefix) else
    var native:=N.Restore(N.Capture(prior.native),i.decoded.native,true,true);
    var context:=Context(native,prior.silent,prior.version,i.decoded.handle);
    var ok:=i.parserResult>0;
    Result(ok,if ok then i.decoded.native.race else output,context,
      prefix+[Capture,SetSilent,Parse(false)]+(if ok then [CopyOutput] else [])+[Restore,RestoreSilentVersion])
  }
  lemma WrapperRestoresOwned(prior: Context,output: S.Bytes,i: Inputs)
    requires N.Shape(prior.native) && N.Shape(i.decoded.native) && |output|==192
    ensures Read(prior,output,i).context.native.race==prior.native.race
    ensures Read(prior,output,i).context.native.password==prior.native.password
    ensures Read(prior,output,i).context.native.cache==prior.native.cache
    ensures Read(prior,output,i).context.native.path==prior.native.path
    ensures Read(prior,output,i).context.native.newFlag==prior.native.newFlag
    ensures Read(prior,output,i).context.silent==prior.silent && Read(prior,output,i).context.version==prior.version
    ensures !Read(prior,output,i).success ==> Read(prior,output,i).output==output
  {}
  lemma HeaderCloseIsNotGate(prior: Context,output: S.Bytes,i: Inputs)
    requires N.Shape(prior.native) && N.Shape(i.decoded.native) && |output|==192
    ensures Read(prior,output,i).success==Read(prior,output,i.(bofCloseFailed:=!i.bofCloseFailed)).success
    ensures Read(prior,output,i).context==Read(prior,output,i.(bofCloseFailed:=!i.bofCloseFailed)).context
    ensures Read(prior,output,i).output==Read(prior,output,i.(bofCloseFailed:=!i.bofCloseFailed)).output
  {}
}
