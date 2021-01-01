$(function(){

    var bytes = [0, 0, 0, 0, 0, 0, 0, 0];

    var target = $('#checkboxes');

    var x, y, checkbox;
    for(x = 0; x < 8; x++)
    {
        for(y = 0; y < 8; y++)
        {
            checkbox = $('<input></input>', {
                'type': 'checkbox',
                'data-x': x,
                'data-y': y
            });

            target.append(checkbox);
        }
    }

    var check = $('input[type="checkbox"]');

    $(check).each(function(){
        $(this).wrap('<span class="pixel"></span>');
        if($(this).is(':checked')) {
            $(this).parent().addClass('selected');
        }
    });
    
    $(check).click(function(){
        $(this).parent().toggleClass('selected');
    });
   
    target.width(checkbox.outerWidth() * 8 + 8);

    target.on('change', 'input:checkbox', function(){

        var $this = $(this),
            x = $this.data('x'),
            y = $this.data('y'),
            checked = $this.prop('checked');

        bytes[y] = bytes[y] ^ (1 << x);
        var data = '';
        for (i = 0; i < 8; i++)
        {
            data += '0x' + ('0' + bytes[i].toString(16)).slice(-2);
            if (i < 7)
            {
                data += ', ';
            }
        }
        $('#data').val(data);
    });

});
