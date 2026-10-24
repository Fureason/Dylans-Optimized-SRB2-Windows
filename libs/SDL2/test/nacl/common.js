// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.



var isTest = false;



var isRelease = true;







var common = (function() {

  function isHostToolchain(tool) {
    return tool == 'win' || tool == 'linux' || tool == 'mac';
  }

     
                                          
    
                                                                                
                                                                        
                         
     
  function mimeTypeForTool(tool) {

    var mimetype = 'application/x-nacl';
    if (isHostToolchain(tool)) {


      if (isRelease)
        mimetype = 'application/x-ppapi-release';
      else
        mimetype = 'application/x-ppapi-debug';
    } else if (tool == 'pnacl') {
      mimetype = 'application/x-pnacl';
    }
    return mimetype;
  }

     
                                                
    
                                                                                
                                                                        
                                     
     
  function browserSupportsNaCl(tool) {



    if (isHostToolchain(tool)) {
      return true;
    }
    var mimetype = mimeTypeForTool(tool);
    return navigator.mimeTypes[mimetype] !== undefined;
  }

     
                                                                         
    
                                                       
                                                                             
                                                                                
     
  function injectScript(url, onload, onerror) {
    var scriptEl = document.createElement('script');
    scriptEl.type = 'text/javascript';
    scriptEl.src = url;
    scriptEl.onload = onload;
    if (onerror) {
      scriptEl.addEventListener('error', onerror, false);
    }
    document.head.appendChild(scriptEl);
  }

     
                                    
    
                                                     
     
  function runTests(moduleEl) {
    console.log('runTests()');
    common.tester = new Tester();






    common.tester.exitCleanlyIsOK();

    common.tester.addAsyncTest('loaded', function(test) {
      test.pass();
    });

    if (typeof window.addTests !== 'undefined') {
      window.addTests();
    }

    common.tester.waitFor(moduleEl);
    common.tester.run();
  }

     
                                                                           
                      
    
                                                  
                                                                                
                                                                      
                                                          
                                                            
                                                                         
     
  function createNaClModule(name, tool, path, width, height, attrs) {
    var moduleEl = document.createElement('embed');
    moduleEl.setAttribute('name', 'nacl_module');
    moduleEl.setAttribute('id', 'nacl_module');
    moduleEl.setAttribute('width', width);
    moduleEl.setAttribute('height', height);
    moduleEl.setAttribute('path', path);
    moduleEl.setAttribute('src', path + '/' + name + '.nmf');


    if (attrs) {
      for (var key in attrs) {
        moduleEl.setAttribute(key, attrs[key]);
      }
    }

    var mimetype = mimeTypeForTool(tool);
    moduleEl.setAttribute('type', mimetype);






    var listenerDiv = document.getElementById('listener');
    listenerDiv.appendChild(moduleEl);




    moduleEl.offsetTop;


    var isHost = isHostToolchain(tool);
    if (isHost) {
      window.setTimeout(function() {
        moduleEl.readyState = 1;
        moduleEl.dispatchEvent(new CustomEvent('loadstart'));
        moduleEl.readyState = 4;
        moduleEl.dispatchEvent(new CustomEvent('load'));
        moduleEl.dispatchEvent(new CustomEvent('loadend'));
      }, 100);   
    }


    if (isTest) {
      var loadNaClTest = function() {
        injectScript('nacltest.js', function() {
          runTests(moduleEl);
        });
      };



      injectScript('test.js', loadNaClTest, loadNaClTest);
    }
  }

     
                                                                             
                   
    
                                                                         
                                                                      
                                                                           
          
     
  function attachDefaultListeners() {
    var listenerDiv = document.getElementById('listener');
    listenerDiv.addEventListener('load', moduleDidLoad, true);
    listenerDiv.addEventListener('message', handleMessage, true);
    listenerDiv.addEventListener('error', handleError, true);
    listenerDiv.addEventListener('crash', handleCrash, true);
    if (typeof window.attachListeners !== 'undefined') {
      window.attachListeners();
    }
  }

     
                                               
    
                                                                 
     
  function handleError(event) {


    var moduleEl = document.getElementById('nacl_module');
    updateStatus('ERROR [' + moduleEl.lastError + ']');
  }

     
                                                                
    
                                                                       
     
  function handleCrash(event) {
    if (common.naclModule.exitStatus == -1) {
      updateStatus('CRASHED');
    } else {
      updateStatus('EXITED [' + common.naclModule.exitStatus + ']');
    }
    if (typeof window.handleCrash !== 'undefined') {
      window.handleCrash(common.naclModule.lastError);
    }
  }

     
                                           
    
                                                                       
     
  function moduleDidLoad() {
    common.naclModule = document.getElementById('nacl_module');
    updateStatus('RUNNING');

    if (typeof window.moduleDidLoad !== 'undefined') {
      window.moduleDidLoad();
    }
  }

     
                                          
    
                                                                               
                                                                              
                              
    
     
  function hideModule() {


    common.naclModule.style.height = '0';
  }

     
                                          
     
  function removeModule() {
    common.naclModule.parentNode.removeChild(common.naclModule);
    common.naclModule = null;
  }

     
                                                          
    
                                            
                                                            
     
  function startsWith(s, prefix) {


    return s.lastIndexOf(prefix, 0) === 0;
  }

   
  var kMaxLogMessageLength = 20;

   
  var logMessageArray = [];

     
                                               
    
                                                                 
    
                                                
     
  function logMessage(message) {
    logMessageArray.push(message);
    if (logMessageArray.length > kMaxLogMessageLength)
      logMessageArray.shift();

    document.getElementById('log').textContent = logMessageArray.join('\n');
    console.log(message);
  }

     
     
  var defaultMessageTypes = {
    'alert': alert,
    'log': logMessage
  };

     
                                                                   
                                 
    
                                                                 
    
                                                                              
                                            
     
  function handleMessage(message_event) {
    if (typeof message_event.data === 'string') {
      for (var type in defaultMessageTypes) {
        if (defaultMessageTypes.hasOwnProperty(type)) {
          if (startsWith(message_event.data, type + ':')) {
            func = defaultMessageTypes[type];
            func(message_event.data.slice(type.length + 1));
            return;
          }
        }
      }
    }

    if (typeof window.handleMessage !== 'undefined') {
      window.handleMessage(message_event);
      return;
    }

    logMessage('Unhandled message: ' + message_event.data);
  }

     
                                                                              
                                                                                
                                                          
    
                                                  
                                                                                
                                                                      
                                                          
                                                            
                                                                        
     
  function domContentLoaded(name, tool, path, width, height, attrs) {



    updateStatus('Page loaded.');
    if (!browserSupportsNaCl(tool)) {
      updateStatus(
          'Browser does not support NaCl (' + tool + '), or NaCl is disabled');
    } else if (common.naclModule == null) {
      updateStatus('Creating embed: ' + tool);



      width = typeof width !== 'undefined' ? width : 200;
      height = typeof height !== 'undefined' ? height : 200;
      attachDefaultListeners();
      createNaClModule(name, tool, path, width, height, attrs);
    } else {




      updateStatus('Waiting.');
    }
  }

   
  var statusText = 'NO-STATUSES';

     
                                                                        
                                                             
    
                                                                               
                                                                       
                      
     
  function updateStatus(opt_message) {
    if (opt_message) {
      statusText = opt_message;
    }
    var statusField = document.getElementById('statusField');
    if (statusField) {
      statusField.innerHTML = statusText;
    }
  }


  return {
     
    naclModule: null,

    attachDefaultListeners: attachDefaultListeners,
    domContentLoaded: domContentLoaded,
    createNaClModule: createNaClModule,
    hideModule: hideModule,
    removeModule: removeModule,
    logMessage: logMessage,
    updateStatus: updateStatus
  };

}());



document.addEventListener('DOMContentLoaded', function() {
  var body = document.body;


  if (body.dataset) {
    var loadFunction;
    if (!body.dataset.customLoad) {
      loadFunction = common.domContentLoaded;
    } else if (typeof window.domContentLoaded !== 'undefined') {
      loadFunction = window.domContentLoaded;
    }


    var searchVars = {};
    if (window.location.search.length > 1) {
      var pairs = window.location.search.substr(1).split('&');
      for (var key_ix = 0; key_ix < pairs.length; key_ix++) {
        var keyValue = pairs[key_ix].split('=');
        searchVars[unescape(keyValue[0])] =
            keyValue.length > 1 ? unescape(keyValue[1]) : '';
      }
    }

    if (loadFunction) {
      var toolchains = body.dataset.tools.split(' ');
      var configs = body.dataset.configs.split(' ');

      var attrs = {};
      if (body.dataset.attrs) {
        var attr_list = body.dataset.attrs.split(' ');
        for (var key in attr_list) {
          var attr = attr_list[key].split('=');
          var key = attr[0];
          var value = attr[1];
          attrs[key] = value;
        }
      }

      var tc = toolchains.indexOf(searchVars.tc) !== -1 ?
          searchVars.tc : toolchains[0];




      if (configs.indexOf(searchVars.config) !== -1)
        var config = searchVars.config;
      else if (configs.indexOf('Release') !== -1)
        var config = 'Release';
      else
        var config = configs[0];

      var pathFormat = body.dataset.path;
      var path = pathFormat.replace('{tc}', tc).replace('{config}', config);

      isTest = searchVars.test === 'true';
      isRelease = path.toLowerCase().indexOf('release') != -1;

      loadFunction(body.dataset.name, tc, path, body.dataset.width,
                   body.dataset.height, attrs);
    }
  }
});
