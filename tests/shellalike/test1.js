#!/usr/bin/gsr -zzdd
const sh = require("shellalike");

function io_test_cat() {
  var cat = new sh.Process("cat|cat");
  print(cat);
  for each(var each in ['one', 'two', 'three'])
    cat.write(each+'\n');
  cat.flush();
  for (var x in cat) print(x.trim());
}
function io_test_close() {
  var cat = new sh.Process("cat|cat");
  print(cat);
  for each(var each in ['one', 'two', 'three'])
    cat.write(each+'\n');
  cat.close();
  for (var x in cat) print(x.trim());
}
function io_test_tail() {
  var log = sh.exec("tail /var/log/auth.log").exec("rot13");
  for (var line in log)
    print(line.trim());
}
function test_out_file() {
  sh.exec("echo SUCCESS > EVER").exec();
}
function test_no_op() {
  // TODO make work!
  var noop = sh.exec("echo FAILURE > TEST");
}
function test_writing() {
  var rot13 = sh.exec("");
}
function test_graph() {
  var x = sh.exec("one").exec("two").exec(function(src){for(var each in src)yield '+++'+each+'+++'});
  print('@@', x.pipeline());
  //sh.ExecAPI.pipeline(x);
}
function test1(t) {
  var x = new sh.Digraph;
  var el1 = x.addElement({'name':'alice','toString':function()this.name});
  var el2 = x.addElement({'name':'bob','toString':function()this.name});
  t.note('added two elements:');
  t.note('\t', el1);
  t.note('\t', el2);
  var snk1 = el1.addPad('snk', {'name':'chloe','toString':function()this.name});
  var src1 = el1.addPad('src', {'name':'danielle','toString':function()this.name});
  var snk2 = el2.addPad('snk', {'name':'eleanor','toString':function()this.name});
  var src2 = el2.addPad('src', {'name':'francesca','toString':function()this.name});
  t.note('added four pads:');
  t.note('\t', snk1);
  t.note('\t', src1);
  t.note('\t', snk2);
  t.note('\t', src2);
  t.eq(x.isCyclic(), false);
  t.eq(x.isLinear(), false);
  snk2.link(src1);
  t.note('linked two of the pads:');
  t.note('\t', snk1);
  t.note('\t', src1);
  t.note('\t', snk2);
  t.note('\t', src2);
  t.eq(x.isCyclic(), false);
  t.eq(x.isLinear(), true);
  snk1.link(src2);
  t.note('linked the final two pads:');
  t.note('\t', snk1);
  t.note('\t', src1);
  t.note('\t', snk2);
  t.note('\t', src2);
  t.eq(x.isCyclic(), true);
  t.eq(x.isLinear(), false);
}
function test2() {
  var x = new sh.Pipeline;
  x.add("tail -f /var/log/auth.log");
  x.add("rot13");
  var shape = x.shape();
  if (shape !== 'all external')
    throw new Error('shape "'+shape+'" !== "all external"');
}
function test3() {
  var x = new sh.Pipeline;
  x.add(function(src){yield 'one\n';yield 'two\n';yield 'three\n'});
  x.add(function(src){for each(let each in src)yield '"'+each+'"'})
  x.add(function(src){for each(let each in src) print(each)});
  var shape = x.shape();
  if (shape !== 'all internal')
    throw new Error('shape "'+shape+'" !== "all internal"');
}
function test4() {
  var x = new sh.Pipeline;
  x.add("tail -f /var/log/auth.log");
  x.add("rot13");
  x.add(function(src){for(var each in src)yield src});
  var shape = x.shape();
  if (shape !== 'to internal')
    throw new Error('shape "'+shape+'" !== "to internal"');
}
function test5() {
  var x = new sh.Pipeline;
  x.add(function(src){yield 'one\n';yield 'two\n';yield 'three\n'});
  x.add("rot13");
  x.add("cat > output-test");
  var shape = x.shape();
  if (shape !== 'to external')
    throw new Error('shape "'+shape+'" !== "to external"');
}
function test6() {
  var x = new sh.Pipeline;
  x.add(function(src){yield 'one\n';yield 'two\n';yield 'three\n'});
  x.add("rot13");
  x.add(function(src){for each(let each in src) print(each)});
  var shape = x.shape();
  if (shape !== 'interleaved')
    throw new Error('shape "'+shape+'" !== "interleaved"');
}
function test7() {
  var shape = sh.exec("cat test-file").exec("rot13").exec(function(src){for each(let each in src)print(each)})._pipeline.shape();
  if (shape !== 'to internal')
    throw new Error('shape "'+shape+'" !== "to internal"');
}
function test8() {
  sh.exec(function(){yield'ONE';yield'TWO';yield'THREE'}).print();
}
function test9() {
  sh.exec("cat tests/shellalike/test1.js").exec("rot13").rtrim().print();
}

var args = Array.apply(null, arguments);
var harness = {
  'eq': function() {
      if (arguments.length < 2) throw 'too few arguments to testUtils.eq()';
      for(var i=1, l=arguments.length; i<l; i++) {
          if (('object' == typeof arguments[0]) && ('object' == typeof arguments[i])) {
              var vals0 = values(arguments[0]);
              var valsi = values(arguments[i]);
              if (vals0.length != valsi.length) {
                  print('OBJECT SIZE MISMATCH');
                  print('V1 "'+vals0+'"');
                  print('V2 "'+valsi+'"');
                  return false;
              }
              for (var j=0, jl=vals0.length; j<jl; j++) {
                  /* TODO recursion instead of one level-deep bs */
                  if (vals0[j] !== valsi[j]) {
                      print('OBJECT VALUE MISMATCH');
                      print('V1 "'+arguments[0]+'"');
                      print('V2 "'+arguments[i]+'"');
                      return false;
                  }
              }
          } else {
              if (arguments[0] !== arguments[i]) {
                  print('VALUE MISMATCH');
                  print('V1 "'+arguments[0]+'"');
                  print('V2 "'+arguments[i]+'"');
                  return false;
              }
          }
      }
      return true;
  },
  'test': function() {
    var failed = 0;
    for(let i=0;i<arguments.length;i++) {
      let test = arguments[i];
      try { 
        harness.note("RUNNING TEST:", test.name);
        test(this)
      } catch (e) {
        failed++;
        print('FAILED:', test.name);
        print('\nuncaught exception\n', e, '\n\n', 'backtrace:\n', e.stack);
      }
    }
    return failed;
  }
}

/* Note API */
if (args.indexOf('-v') >= 0)
  harness.note = print;
else
  harness.note = function()void 0;

try {
  var failed = harness.test(test1, test2, test3, test4, test5, test6, test7, test8, test9);
  if (failed) {
    print(failed, "tests failed");
    //throw -1;
  }
  else {
    print('success');
  }
} catch (e) {
  print('\nuncaught exception\n', e, '\n\n', 'backtrace:\n', e.stack);
  //throw(-1);
}
//print(sh.ExecAPI.splice(sh.exec('tail -f /var/log/syslog'), sh.exec('cat > whatever')));
