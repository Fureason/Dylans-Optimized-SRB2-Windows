Module['arguments'] = ['0'];

Module['preRun'].push(function()
{
    Module['print']("Waiting for gamepad...");
    Module['addRunDependency']("gamepad");
    window.addEventListener('gamepadconnected', function()
    {

        Module['removeRunDependency']("gamepad");
    }, false);


    if(!!navigator.webkitGetGamepads)
    {
        var timeout = function()
        {
            if(navigator.webkitGetGamepads()[0] !== undefined)
                Module['removeRunDependency']("gamepad");
            else
                setTimeout(timeout, 100);
        }
        setTimeout(timeout, 100);
    }
});
