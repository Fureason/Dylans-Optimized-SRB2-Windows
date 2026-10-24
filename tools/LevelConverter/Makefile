


SRC=levelconverter.c
OBJ=$(SRC:.c=.o)# replaces the .c from SRC with .o
EXE=LevelConverter

.PHONY : all      
all: $(EXE)       

$(EXE): $(OBJ)  
	$(CC) $(OBJ) $(LDFLAGS) -o $@

.PHONY : clean    
clean:
	-$(RM) $(OBJ) $(EXE)
