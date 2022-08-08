var injection_object;
var web_channel = new QWebChannel(qt.webChannelTransport, function (channel) {
    injection_object = channel.objects.injection_object;

    // получаем значение свойства injection_objectis PdfView и сохраняем его в переменную
    var isPdfView = false;
    injection_object.isPdfView( function(value) {
        isPdfView = value;
    });

    // подписываемся на изменение свойств
    injection_object.optionsChanged.connect(function() {
         updateToolbarVisability();
    });

    /*
    function loop(node){
        console.log(node)
        var nodes = node.childNodes;
        for (var i = 0; i <nodes.length; i++){
            if(!nodes[i]){
                continue;
            }

            if(nodes[i].childNodes.length > 0){
                loop(nodes[i]);
            }
        }
    }
    loop(document);*/

    // pdf верхний тулбар
    var toolbar = document.getElementById("toolbar");
    // pdf zoom тулбар
    var zoom_toolbar = document.getElementById("zoom-toolbar");

    if (!toolbar)
        console.log("web_channel_injection.js error - pdf toolbar not found")
    if (!zoom_toolbar)
        console.log("web_channel_injection.js error - pdf zoom_toolbar not found")

    // скрываем тулбары в pdf если надо
    function updateToolbarVisability() {
        injection_object.isHidePdfToolbar( function(value) {
            if (value)
                toolbar.style.visibility = "hidden";
            else
                toolbar.style.visibility = "visible";
        });
    };

    if (toolbar) {
        // управление поворотами
        injection_object.rotateRight.connect(function() {
            if (!isPdfView)
                return;
            toolbar.rotateRight();
        });
        injection_object.rotateLeft.connect(function() {
            if (!isPdfView)
                return;
            toolbar.rotateRight();
            toolbar.rotateRight();
            toolbar.rotateRight();
        });

        // текущая страница при начальной загрузке
        setTimeout(function (e) {
            // не знаю как это сделать по человечески
            // при попытке прочитать toolbar.pageNo сразу - оно еще не определено
            if (!isPdfView)
                return;
            injection_object.onCurrentPageChanged(toolbar.pageNo,toolbar.docLength);
        },1000);
        // текущая страница при прокрутке
        window.addEventListener("scroll",
            function (e) {
                if (!isPdfView)
                    return;
                injection_object.onCurrentPageChanged(toolbar.pageNo,toolbar.docLength);
        });
        // установка текущей страницы
        injection_object.setCurrentPage.connect(function(page) {
            if (!isPdfView)
                return;
            // такое впечатление что это делать нельзя вообще
            // toolbar.pageNo = page;
        });
    };

    if (zoom_toolbar) {
        // скрытие тулбара зума
        injection_object.isHidePdfZoom( function(value) {
            if (value)
                zoom_toolbar.style.visibility = "hidden";
            else
                zoom_toolbar.style.visibility = "visible";
        });

        // управление зумом
        injection_object.setFittingType.connect(function(type) {
            if (!isPdfView)
                return;

            var f_type;

            /*! enum WebEngineView::FittingType
             * Page = 1,
             * Width = 2,
             * Height = 3 */

            if (type === 1)
                f_type = 'fit-to-page';
            else if (type === 2)
                f_type = 'fit-to-width';
            else if (type === 3)
                f_type = 'fit-to-height';
            else {
                console.log("setFittingType - bad type: ", type);
                return;
            }

            zoom_toolbar.forceFit(f_type);
        });
        injection_object.zoomIn.connect(function() {
            if (!isPdfView)
                return;
            zoom_toolbar.zoomIn();
        });
        injection_object.zoomOut.connect(function() {
            if (!isPdfView)
                return;
            zoom_toolbar.zoomOut();
        });
    };

    if (toolbar) {
        updateToolbarVisability();
    }

    // отслеживание нажатия кнопок
    document.body.addEventListener("keypress",
        function (e) {
            injection_object.onKeyPressed(e.target.id, e.key);
        },
        false);
    // отслеживание нажатия мыши
    document.body.addEventListener("click",
        function (e) {
            injection_object.onMouseClicked(e.target.id, e.altKey, e.ctrlKey, e.shiftKey, e.button);
        },
        false);

    // обсервер скорее всего не понадобится, но пусть пока тут будет его рабочий код
    var callback = function(mutationsList, observer) {
        mutationsList.forEach((mutation) => {
        switch(mutation.type) {
              case 'childList':
                /* One or more children have been added to and/or removed
                   from the tree; see mutation.addedNodes and
                   mutation.removedNodes */
                break;
              case 'attributes':
                /* An attribute value changed on the element in
                   mutation.target; the attribute name is in
                   mutation.attributeName and its previous value is in
                   mutation.oldValue */
                break;
            }
        });
    };

    var observer = new MutationObserver(callback);
    observer.observe(document,
          { subtree: true,
            attributes: true,
            childList: true,
            characterData: true,
            characterDataOldValue: true
          });
});
