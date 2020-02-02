.386p ; Разрешение трансляции всех, в том
			; числе привилегированных команд МП 386
			; и 486

descr struc  ; Cтруктура для описания дескриптора сегмента
	limit 	    dw 0	; Граница (биты 0..15)
	base_l  	dw 0	; База, биты 0..15
	base_m 	    db 0	; База, биты 16..23
	attr_1 	    db 0	; Байт атрибутов 1
	arrt_2 	    db 0	; Граница(биты 16..19) и атрибуты 2
	base_h 	    db 0	; База, биты 24..31
descr ends

intr struc ; Структура для описания дескрипторов прерываний
	offs_l 	    dw 0    ; Смещение обработчика, нижняя часть (биты 0..15)
	sel	        dw 0	; Селектор сегмента команд
	rsrv        db 0    ; Зарезервировано
	attr	    db 0    ; Атрибуты
	offs_h 	    dw 0    ; Смещение обработчика, верхняя часть (биты 16..31)
intr ends

stack_seg segment para stack 'STACK'
		stack_start	    db 100h dup(?)
		stack_l = $-stack_start	; длина стека для инициализации ESP
stack_seg ENDS

; Защищенный режим
PM_segment segment para public 'CODE' use32
		assume CS:PM_segment

		GDT	label	    byte ; Таблица глобальных дескрипторов
		gdt_null        descr <0,0,0,0,0,0> ; Нулевой дескриптор
		gdt_flatDS      descr <0FFFFh,0,0,92h,0CFh,0> ; переключение в 32-х битную модель памяти flat
															; с лимитом в 4 Гб и страничной адресацией
		gdt_16bitCS	    descr <RM_segment_size-1,0,0,98h,0,0> ; 16-битный 64-килобайтный сегмент кода с базой RM_segment
		gdt_32bitCS	    descr <PM_segment_size-1,0,0,98h,0CFh,0> ; 32-битный 4-гигабайтный сегмент кода с базой PM_segment
		gdt_32bitDS	    descr <PM_segment_size-1,0,0,92h,0CFh,0> ; 32-битный 4-гигабайтный сегмент данных с базой PM_segment
		gdt_32bitSS	    descr <stack_l-1,0,0,92h,0CFh,0> ; 32-битный 4-гигабайтный сегмент данных с базой stack_seg
		gdt_size = $-GDT ; размер таблицы GDT+1байт (на саму метку)
		; сегмент видеобуфера рассчитывается как смещение

		gdtr            dw gdt_size-1 ; Лимит GDT
				        dd ? ; Линейный адрес GDT

		; Имена для селекторов
		SEL_flatDS      equ 8
		SEL_16bitCS     equ 16
		SEL_32bitCS     equ 24
		SEL_32bitDS     equ 32
		SEL_32bitSS     equ 40

		IDT	label	    byte ; Таблица дескрипторов прерываний IDT
        trap1           intr 13 dup (<0, SEL_32bitCS,0, 8Fh, 0>) ; Первые 32 элемента таблицы (зарезервированы под исключения)		
		trap2           intr 18 dup (<0, SEL_32bitCS,0, 8Fh, 0>) ; Первые 32 элемента таблицы (зарезервированы под исключения)
        trap13          intr <0, SEL_32bitCS,0, 8Fh, 0> ; Исключение общей защиты
		int08           intr <0, SEL_32bitCS,0, 8Eh, 0> ; Дескриптор прерывания от таймера
		int09           intr <0, SEL_32bitCS,0, 8Eh, 0> ; Дескриптор прерывания от клавиатуры
		idt_size = $-IDT ; Размер IDT

		idtr            dw idt_size-1 ; Лимит IDT
				        dd ? ; Линейный адрес IDT

		idtr_real       dw 3FFh,0,0 ; содержимое регистра IDTR в реальном режиме

		mes_prot        db 'Protected mode!$'
		mes_real        db 'Real mode!$'

        master          db 0 ; Маска прерываний ведущего контроллера
        slave	        db 0 ; Ведомого
		
		; Таблица символов ASCII для перевода из скан кода в код ASCII.
		; Номер скан кода = номеру соответствующего элемента в таблице:
		scan2ascii      db 0,1Bh,'1','2','3','4','5','6','7','8','9','0','-','=',8
						db 0,0,0,0,0,0,0,0,0,0,0,0
						db 0,0,0,0,0,0,0,0,0,0,0,0
						db 0,0,0,0,0,0,0,0,0,0,0,0
						db 0,0,0,0,0,0,0,0,0,0,0,0

		screen_addr	    dd 1E0h ; Позиция печати вводимого текста
		timer         dd 0 ; Счетчик прошедших тиков таймера

	; Макрос печати строки
	output_str macro string
	    local screen
	screen:
		mov             AL,word ptr string[SI] ; Символ для вывода со смещением SI
		mov             ES:[EBP],AL ; Вывод в видеобуфер
		add             EBP,2 ; Смещаемся в видеобуфере
		inc             SI ; Следующий символ строки
		loop            screen ; Цикл вывода на экран
	endm

	; Макрос печати числа
	output_num macro
	    local cycle, num, print
	cycle:
		mov             DL,AL  ; Кладём в DL текущее значение AL (самый младший байт ЕАХ)
		and             DL,0Fh ; Оставляем от него одно 16ричное число (последняя цифра, последние 4 бита)
		cmp             DL,10
		jl              num
		sub             DL, 10 ; Превращаем число в букву
		add             DL,'A'
		jmp             print
	num:
		add             DL,'0' ; Превращаем это цифру в символ
	print:
		mov             ES:[EBP],DL	; Выводим в видеобуфер
		ror             EAX,4	; Циклически двигаем биты в ЕАХ - таким образом, после всех перестановок,
							        ; ЕАХ окажется тем же что и в начале, нет необходимости на PUSH; POP
		sub             EBP,2	; Смещаем позицию вывода
		loop            cycle	; Цикл
	endm

	; Точка входа в 32-битный защищенный режим
	PM_entry:
		; Установить 32-х битный стек и другие регистры (загруженные в дескрипторы)
		mov	            AX,SEL_flatDS
		mov	            DS,AX
		mov	            ES,AX
		mov	            AX,SEL_32bitSS
		mov	            EBX,stack_l
		mov	            SS,AX
		mov	            ESP,EBX

		; Разрешить прерывания
		sti

		; Protected mode - вывод на экран
		push            EBP
		xor             EAX,EAX
		mov             EBP,0
		add             EBP,0B8000h ; Начальное смещение на экране
		mov             ECX,15 ; Число выводимых символов
		mov             SI,0 ; Итератор (смещение от начала)
		output_str      mes_prot
		pop             EBP

		; Memory - вывод на экран
		push            EBP
		xor             EAX,EAX
		mov             EBP,160 ; Одна строка 160 символов
		add             EBP,0B8000h ; Начальное смещение на экране
		mov             ECX,7 ; Число выводимых символов
		mov             SI,0 ; Итератор (смещение от начала)
		pop             EBP

        ; Timer - вывод на экран
		push            EBP
		xor             EAX,EAX
		mov             EBP,160 ; Одна строка 160 символов
		add             EBP,0B8000h ; Начальное смещение на экране
		mov             ECX,6 ; Число выводимых символов
		mov             SI,0 ; Итератор (смещение от начала)
		pop             EBP

		; Cчитаем количество доступной памяти и печатаем его на экран
		call            compute_memory

		; Крутимся в бесконечном цикле, периодически натыкаясь на прерывания клавиатуры и таймера
		; Выход из цикла - по нажатию Enter
		jmp             short $

	; Обработчик остальных исключений
	dummy_exc proc
		mov             AX,1111h
		jmp             enter_pressed
	dummy_exc endp

	; Обработчик исключения общей защиты
	exc13 proc
		mov             AX,0Dh
		jmp             enter_pressed
	exc13 endp

	; Обработчик прерывания таймера
	int08_handler:
		push            EAX ; Это аппаратное прерывание, сохраняем регистры
		push            EBP
		push            ECX
		push            DX
		mov             EAX,timer

		mov             EBP,50 ; Ещё 30h - поскольку число печатается справа-налево
		mov             ECX,4	; Число выводимых символов
		add             EBP,0B8000h ; Начальное смещение на экране
                        ; 0B8000h - смещение видеобуффера относительно начала сегмента.
        cycle1:
            mov         DL,AL  ; Кладём в DL текущее значение AL (самый младший байт ЕАХ)
            and         DL,0Fh ; Оставляем от него одно 16ричное число (последняя цифра)
            cmp         DL,10
            jl          number1
            sub         DL, 10 ; Превращаем число в букву
            add         DL,'A'
            jmp         print1
        number1:
            add         DL,'0' ; Превращаем эту цифру в символ
        print1:
            mov         ES:[EBP],DL	; Выводим в видеобуфер
            ror         EAX,4	; Циклически двигаем биты в ЕАХ - таким образом, после всех перестановок,
                                ; ЕАХ окажется тем же что и в начале, нет необходимости на PUSH; POP
            sub         EBP,2	; Смещаем позицию вывода
            loop        cycle1	; Цикл
		; Инкремент
		inc             EAX
		mov             timer,EAX

		; Посылаем сигнал EOI контроллеру прерываний и восстанавливаем регистры
		mov	            AL,20h
		out	            20h,AL
		pop             DX
		pop             ECX
		pop             EBP
		pop             EAX
		; Выходим из прерывания
		iretd

	; Обработчик прерывания клавиатуры
	int09_handler:
		push            EAX ; Это аппаратное прерывание, сохраняем регистры
		push            EBX
		push            ES
		push            DS

		in	            AL,60h ; Прочитать скан-код нажатой клавиши из порта клавиатуры
		cmp	            AL,1Ch ; Сравниваем с кодом enter
		je              enter_pressed ; Если enter, выходим в реальный режим
		cmp             AL,0Eh ; Сравним какой скан-код пришел: нажатой клавиши или нет?
		ja              skip_translate ; Если нет, то ничего не выводим
		mov             bx,SEL_32bitDS ; Иначе
		mov             DS,bx ; DS:EBX - таблица для перевода скан-кода в ASCII
		mov             EBX,offset scan2ascii 
		xlatb ; Преобразовать
		mov             bx,SEL_flatDS
		mov             ES,bx ; ES:EBX - адрес текущей
		mov             EBX,screen_addr ; позиции на экране
		cmp             AL,8 ; Если не была нажата Backspace
		je              bs_pressed
		mov             ES:[EBX+0B8000h],AL ; Вывести символ на экран
		add             dword ptr screen_addr,2 ; Увеличить адрес позиции на 2
		jmp             short skip_translate
	bs_pressed: ; Иначе
		mov             AL,' ' ; нарисовать пробел
		sub             EBX,2 ; в позиции предыдущего символа
		mov             ES:[EBX+0B8000h],AL ; Вывести символ на экран
		mov             screen_addr,EBX ; и сохранить адрес предыдущего символа как текущий
	skip_translate:
		; Разрешить работу клавиатуры
		in	            AL,61h
		or	            AL,80h
		out	            61h,AL
		; Посылаем сигнал EOI контроллеру прерываний
		mov	            AL,20h
		out	            20h,AL
		; Восстановить регистры и выйти
		pop             DS
		pop             ES
		pop             EBX
		pop	            EAX
		iretd

	; Сюда передается управление из обработчика int09h при нажатии enter
	enter_pressed:
		; Разрешить работу клавиатуры, послать EOI и восстановить регистры.
		in	            AL,61h
		or	            AL,80h
		out	            61h,AL
		mov	            AL,20h
		out	            20h,AL
		pop             DS
		pop             ES
		pop             EBX
		pop	            EAX
		; Запрет прерываний
		cli

		; Возврат в реальный режим
		db	            0EAh
		dd	            offset RM_return
		dw	            SEL_16bitCS

    ;функция подсчета доступной памяти
	compute_memory proc
		push            DS ; Сохранение регистров
		mov	            AX,SEL_flatDS ; Кладем в него сегмент на 4 ГБ - все доступное виртуальное адресное пространство
		mov	            DS,AX
		mov	            EBX,100001h	; Пропускаем первый мегабайт сегмента (потому, что в противном случае может произойти
																	; попытка редактирования процедуры собственного кода)
		mov	            DL,10010011b ; Проверочный байт

		mov	            ECX,0FFEFFFFFh ; В ECX кладём количество оставшейся памяти - чтобы не было переполнения
											 ; лимит в 4 Гб = 4096 Мб, без одного Мб = 4293918719 байт

		; В цикле считаем память
	check_byte:
		mov	            DH,DS:[EBX]	; Сохраняем в DH текущее значение по некоторому байту памяти
		mov         	DS:[EBX],DL ; Кладём некоторое значение в этот байт
		cmp	            DS:[EBX],DL ; Проверяем - считается обратно то же DL, если свопали, то перед нами память
		jnz	            mem_end	; если не совпали - то мы достигли конца памяти, выходим из цикла
		mov	            DS:[EBX], DH ; если не достигли - кладём обратно сохранённое значение
		inc	            EBX	; Проверяем следующий байт и тд (размер памяти можно задать в настройках DOSBOX по умолчанию 16 Мб)
		loop            check_byte

	mem_end:            ; Достигли конца памяти
		pop	            DS ; Восстанавливаем регистры
		xor	            EDX,EDX
		mov	            EAX,EBX ; В EBX лежит количество посчитанной памяти в байтах; кладём его в EAX,
		mov	            EBX,100000h ; делим на 1 Мб, чтобы получить результат в Мб
		div	            EBX

		push            ECX ; Сохранение регистров
		push            DX
		push            EBP

		mov             EBP,70 ; Указываем смещение в видеопамяти относительно начала экрана
								 ; (30h поскольку число печатается справа-налево)
		mov             ECX,3	; Число выводимых символов
		add             EBP,0B8000h ; Начальное смещение на экране
        ; 0B8000h - смещение видеобуффера относительно начала сегмента
        cycle2:
            mov         DL,AL	; Кладём в DL текущее значение AL (самый младший байт ЕАХ)
            and         DL,0Fh ; Оставляем от него одно 16ричное число (последняя цифра)
            cmp         DL,10
            jl      number2
            sub         DL, 10 ; Превращаем число в букву
            add         DL,'A'
            jmp         print2
        number2:
            add         DL,'0' ; Превращаем это цифру в символ
        print2:
            mov         ES:[EBP],DL	; Выводим в видеобуфер
            ror         EAX,4	; Циклически двигаем биты в ЕАХ - таким образом, после всех перестановок,
                                ; ЕАХ окажется тем же что и в начале, нет необходимости на PUSH; POP
            sub         EBP,2	; Смещаем позицию вывода
            loop        cycle2	; Цикл
		sub             EBP,0B8000h
		pop             EBP ; Восстанавливаем регистры
		pop             DX
		pop             ECX

		ret
	compute_memory	endp

	PM_segment_size = $-GDT
PM_segment ENDS

RM_segment segment para public 'CODE' use16
	assume CS:RM_segment, DS:PM_segment, SS:stack_seg
	start:
		; Очистить экран
		mov	            AX,3
		int	            10h
		; Подготовить сегментные регистры
		push            PM_segment
		pop             DS

		; Вычислить базы для всех используемых дескрипторов сегментов
		xor	            EAX,EAX
		mov	            AX,RM_segment
		shl	            EAX,4	; Сегменты объявлены как PARA, нужно сдвинуть на 4 бита для выравнивания по границе параграфа
		mov	            word ptr gdt_16bitCS+2,AX ; Базой 16bitCS будет RM_segment (base_l)
		shr	            EAX,16
		mov	            byte ptr gdt_16bitCS+4,AL ; (base_m)
		mov	            AX,PM_segment
		shl	            EAX,4
		push            EAX ; Для вычисления адреса idt
		push            EAX ; Для вычисления адреса gdt
		mov	            word ptr GDT_32bitCS+2,AX ; Базой всех 32bit будет (base_l)
		mov	            word ptr GDT_32bitSS+2,AX ; (base_l)
		mov	            word ptr GDT_32bitDS+2,AX ; (base_l)
		shr	            EAX,16
		mov	            byte ptr GDT_32bitCS+4,AL ; (base_m)
		mov	            byte ptr GDT_32bitSS+4,AL ; (base_m)
		mov	            byte ptr GDT_32bitDS+4,AL ; (base_m)

		; Вычислим линейный адрес GDT
		pop             EAX
		add	            EAX,offset GDT ; В EAX будет полный линейный адрес GDT (адрес сегмента + смещение GDT относительно него)
		mov	            dword ptr gdtr+2,EAX ; Кладём полный линейный адрес в младшие 4 байта переменной gdtr
		mov             word ptr gdtr, gdt_size-1 ; В старшие 2 байта заносим размер gdt, из-за определения gdt_size (через $)
																  ; настоящий размер на 1 байт меньше
		; загрузим GDT
		lgdt            fword ptr gdtr

		; Аналогично вычислим линейный адрес IDT
		pop	            EAX
		add	            EAX,offset IDT
		mov	            dword ptr idtr+2,EAX
		mov             word ptr idtr,idt_size-1

		; Заполним смещение в дескрипторах прерываний
		mov             EAX,offset dummy_exc
		mov             trap1.offs_l,AX
		shr             EAX,16
		mov             trap1.offs_h,AX
		mov             EAX,offset exc13
		mov             trap13.offs_l,AX
		shr             EAX,16
		mov             trap13.offs_h,AX
		mov             EAX,offset dummy_exc
		mov             trap2.offs_l,AX
		shr             EAX,16
		mov             trap2.offs_h,AX
		mov	            EAX,offset int08_handler ; Прерывание системного таймера
		mov	            int08.offs_l,AX
		shr	            EAX,16
		mov	            int08.offs_h,AX
		mov	            EAX,offset int09_handler ; Прерывание клавиатуры
		mov	            int09.offs_l,AX
		shr	            EAX,16
		mov	            int09.offs_h,AX

		; Cохраним маски прерываний контроллеров
		in              AL,21h ; ведущего, 21h - номер шины, in на неё даст нам набор масок (флагов)
		mov	            master,AL ; сохраняем в переменной master
		in              AL,0A1h ; ведомого - аналогично, in даёт набор масок для ведомого
		mov	            slave,AL

		; Перепрограммируем ведущий контроллер прерываний
		; Базовый теперь 32=20h
		mov             DX,20h ; Порт контроллера
		mov	            AL,11h ; Командное слово инициализации ведущего контроллера (так как 2 контроллера прерываний)
		out	            DX,AL  ; Первое слово в первый порт - 20h, остальные слова во 2
		inc             DX     ; Последующие слова инициализации контроллера записываются в порт 21h
		mov	            AL,20h ; 2: Базовый вектор (начальное смещение для обработчика) 
                                   ; теперь 32 (20h) (вслед за векторами исключений)
		out	            DX,AL  ; Указываем, что аппаратные прерывания будут обрабатываться начиная с 32го (20h)
		mov	            AL,4   ; 3: Определение номера входа, к которому подключен 
                                   ; ведомый контроллер(к уровню 2, установить 2 бит)
		out	            DX,AL
		mov	            AL,1   ; 4: Указываем, что нужно будет посылать команду завершения обработчика прерывания (EOI)
		out	            DX, AL
		; Запретим все прерывания в ведущем контроллере, кроме IRQ0 (таймер) и IRQ1(клавиатура)
		mov	            AL,0FCh ; Маска прерываний 11111100
		out	            DX,AL

		; Запретим все прерывания в ведомом контроллере
		; В противном случае возникнет исключение - может прийти прерывание для которого у нас не написан обработчик
		mov             DX,0A1h
		mov	            AL,0FFh
		out	            DX,AL

		; Загрузим IDT
		lidt            fword ptr idtr

		; Если мы собираемся работать с 32-битной памятью, стоит открыть A20
		; А20 - адресная линия ("шина")
		; через которую осуществляется доступ ко всей памяти за пределами 1 Мб
		; если не открыть, то память будет битая
		in              AL,92h ; Установка бита 2 в порт А20
		or              AL,2
		out             92h,AL


		; =======================================================================
		; открытие линии А20
		mov 			AL, 0d1h
		out				64. al
		mov				al, 0DFh
		out 			60h, al

		; закрытие линии А20
		mov 			AL, 0d1h
		out				64. al
		mov				al, 0DDh
		out 			60h, al
		; =======================================================================
		

		; Отключить маскируемые прерывания
		cli
		; и немаскируемые прерывания
		in              AL,70h
		or              AL,80h
		out	            70h,AL

		; Перейти в непосредственно защищенный режим установкой соответствующего бита регистра CR0
		mov	            EAX,CR0
		or              AL,1
		mov	            CR0,EAX

		; Загрузить SEL_32bitCS в регистр CS
		db              66h
		db              0EAh
		dd              offset PM_entry
		dw              SEL_32bitCS
		; Начиная с этой строчки, будет выполняться код защищенного режима PM_entry

	RM_return:
		; Переход в реальный режим
		mov	            EAX,CR0
		and	            AL,0FEh ; сбрасываем флаг защищенного режима
		mov	            CR0,EAX

		; Сбросить очередь и загрузить CS реальным числом
		db              0EAh
		dw              $+4
		dw              RM_segment

		; Восстановить регистры для работы в реальном режиме
		mov	            AX,PM_segment	; Загружаем в сегментные регистры реальные смещения
		mov	            DS,AX
		mov	            ES,AX
		mov	            AX,stack_seg
		mov	            bx,stack_l
		mov	            SS,AX
		mov	            sp,bx

		; Восстанавливаем маски контроллеров прерываний
		; реинициализацию контроллера прерываний проводить не требуется
		mov	            AL,master
		out	            21h,AL
		mov	            AL,slave
		out	            0A1h,AL

		; Загружаем таблицу дескрипторов прерываний реального режима
		lidt            fword ptr idtr_real

		; Разрешаем немаскируемые прерывания
		in              AL,70h
		and	            AL,07FH
		out             70h,AL

	    ; и маскируемые
		sti

		; Очистить экран
		mov	            AX,3
		int	            10h

		; Печать сообщения о выходе из защищенного
		mov             AH, 09h
		mov             EDX, offset mes_real
		int             21h

		; Завершаем программу через int 21h по команде 4Ch
		mov	            AH,4Ch
		int	            21h
	RM_segment_size = $-start ; завершаем сегмент, указываем метку начала для сегмента
RM_segment ENDS

END start