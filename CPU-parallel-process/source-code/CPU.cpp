#include "CPU.h"

CPU(){
    F = new  QProcess(this);
    F->start("F:\2017Spring\ICS\Y86-Lab\CPU-parallel-process\source-code\FDEMW\F.exe");
    connect(F, SIGNAL(readyRead()), this, SLOT(F_ret()));
    D = new  QProcess(this);
    D->start("F:\2017Spring\ICS\Y86-Lab\CPU-parallel-process\source-code\FDEMW\D.exe");
    connect(D, SIGNAL(readyRead()), this, SLOT(D_ret()));
    E = new  QProcess(this);
    E->start("F:\2017Spring\ICS\Y86-Lab\CPU-parallel-process\source-code\FDEMW\E.exe");
    connect(E, SIGNAL(readyRead()), this, SLOT(E_ret()));
    M = new  QProcess(this);
    M->start("F:\2017Spring\ICS\Y86-Lab\CPU-parallel-process\source-code\FDEMW\M.exe");
    connect(M, SIGNAL(readyRead()), this, SLOT(M_ret()));
    W = new  QProcess(this);
    W->start("F:\2017Spring\ICS\Y86-Lab\CPU-parallel-process\source-code\FDEMW\W.exe");
    connect(W, SIGNAL(readyRead()), this, SLOT(W_ret()));
}

void CPU :: prepare(){
    memset(this, 0, sizeof(*this));
    F_predPC = code_head;
    D_icode = E_icode = M_icode = 1;
    D_stat = E_stat = M_stat = Stat = 1;
    D_rA = D_rB = RNONE;
    E_dstE = E_dstM = E_srcA = E_srcB = RNONE;
    M_dstE = M_dstM = RNONE;
    W_dstE = W_dstM = RNONE;
    f_predPC = code_head;
    f_icode = d_icode = e_icode = m_icode = 1;
    f_stat = d_stat = e_stat = m_stat = Stat = 1;
    f_rA = f_rB = RNONE;
    d_dstE = d_dstM = d_srcA = d_srcB = RNONE;
    e_dstE = e_dstM = RNONE;
    m_dstE = m_dstM = RNONE;
}

void CPU :: mem_read(int head, int len, int &data, bool &imem_error){
    data = 0;
    for (int i = 0; i < len; i++)
        if (head + i >= 0){
            int x = mem[head + i];
            x = x & (0xff);
            data =  data ^ (x << (i * 8));
        }else{
            imem_error = 1;
            break;
        }
}

void CPU :: mem_write(int head, int len, int data, bool &imem_error){
    for (int i = 0; i < len; i++)
        if (head + i >= 0){
            mem[head + i] = data & (0xff);
            data >>= 8;
        }else{
            imem_error = 1;
            break;
        }
}

void CPU :: read_in(QString path){
    string st = path.toStdString();
    const char *pch = st.c_str();
    FILE *bin_stream;
    int head = code_head;
    bin_stream = fopen(pch, "r");
    len = fread(bin_code, sizeof(char), MAXLEN, bin_stream);
    for (int i = 0; i < len; i++){
        mem_write(head, 1, bin_code[i], imem_error);
        head++;
    }
}

void CPU :: F_Control(){
    F_stall = (E_icode == IMRMOVL || E_icode == IPOPL) && (E_dstM == d_srcA || E_dstM == d_srcB);
    F_stall |= (D_icode == IRET || E_icode == IRET || M_icode == IRET);
}

void CPU :: D_Control(){
    D_stall = (E_icode == IMRMOVL || E_icode == IPOPL) && (E_dstM == d_srcA || E_dstM == d_srcB);
    D_bubble = (E_icode == IJXX && !e_Cnd);
    D_bubble |= (!((E_icode == IMRMOVL || E_icode == IPOPL) && (E_dstM == d_srcA || E_dstM == d_srcB))) && (IRET == D_icode || IRET == E_icode || IRET == M_icode);
}

void CPU :: E_Control(){
    E_bubble = (E_icode == IJXX && !e_Cnd);
    E_bubble |= (E_icode == IMRMOVL || E_icode == IPOPL) && (E_dstM == d_srcA || E_dstM == d_srcB);
}

void CPU :: M_Control(){
    M_bubble = (m_stat == SADR || m_stat == SINS || m_stat == SHLT);
    M_bubble |= (W_stat == SADR || W_stat == SINS || W_stat == SHLT);
}

void CPU :: W_Control(){
    W_stall = (W_stat == SADR || W_stat == SINS || W_stat == SHLT);
}

void CPU :: Control(){
    F_Control();
    D_Control();
    E_Control();
    M_Control();
    W_Control();
}

void CPU :: Send(){
    if (!F_stall && !F_bubble)
        F_predPC = f_predPC;

    if (!D_stall && !D_bubble){
        D_stat = f_stat;
        D_icode = f_icode;
        D_ifun = f_ifun;
        D_rA = f_rA;
        D_rB = f_rB;
        D_valC = f_valC;
        D_valP = f_valP;
    }

    if (!D_stall && D_bubble){
        D_stat = SBUB;
        D_icode = 1;
        D_ifun = 0;
        D_rA = D_rB = RNONE;
        D_valC = D_valP = 0;
    }

    if (!E_stall && !E_bubble){
        E_stat = d_stat;
        E_icode = d_icode;
        E_ifun = d_ifun;
        E_valC = d_valC;
        E_valA = d_valA;
        E_valB = d_valB;
        E_dstE = d_dstE;
        E_dstM = d_dstM;
        E_srcA = d_srcA;
        E_srcB = d_srcB;
    }

    if (!E_stall && E_bubble){
        E_stat = SBUB;
        E_icode = 1;
        E_ifun = 0;
        E_valA = E_valB = 0;
        E_dstE = E_dstM = RNONE;
        E_srcA = E_srcB = RNONE;
    }

    if (!M_stall && !M_bubble){
        M_stat = e_stat;
        M_icode = e_icode;
        M_Cnd = e_Cnd;
        M_valE = e_valE;
        M_valA = e_valA;
        M_dstE = e_dstE;
        M_dstM = e_dstM;
    }

    if (!M_stall && M_bubble){
        M_stat = SBUB;
        M_icode = 1;
        M_Cnd = 0;
        M_valE = M_valA = 0;
        M_dstE = M_dstM = RNONE;
    }

    if (!W_stall && !W_bubble){
        W_stat = m_stat;
        W_icode = m_icode;
        W_valE = m_valE;
        W_valM = m_valM;
        W_dstE = m_dstE;
        W_dstM = m_dstM;
    }
}

void CPU :: Fetch(){
	sstream ss;
	ss.clear();
	ss << F_predPC << M_icode << M_Cnd << M_valA << W_icode << W_valM;
//	qDebug() << ss;
	string Output;
	ss >> Output;
//	qDebug() << Output;
	F_done = 0;
	F -> write(ss.toUtf8());
}

void CPU :: Decode(){
	sstream ss;
	ss.clear();
	ss << D_stat << D_icode << D_ifun << D_rA << D_rB << D_valC << D_valP;
	ss << M_dstM << M_dstE << M_valE << W_dstM << W_valM << W_dstE << W_valE;
	string Output;
	ss >> Output;
	D_done = 0;
	D -> write(ss.toUtf8());
}

void CPU :: Execute(){
	
}

void CPU :: Memory(){
	
}

void CPU :: Write(){
	
}

void CPU :: F_ret(){
	sstream ss;
	ss.clear();
	ss << F -> readAllStandardOutput();
	ss >> ch;
	if (ch == '?'){
		int head, len, data;
		bool imem_error;
		ss >> head >> len;
		mem_read(head, len, data, imem_error);
		ss.clear();
		ss << data;
		if (imem_error) ss  << 1;
		else ss << 0;
		F -> write(ss.toUtf8());
	}
	else if (ch == '*'){
		ss >> f_stat >> f_icode >> f_ifun >> f_rA >> f_rB >> f_valC >> f_valP >> f_PC >> f_predPC;
	}
	else
		qDebug << "F Error :" << ch;
}

void CPU :: D_ret(){
	sstream ss;
	ss.clear();
	ss << D -> readAllStandardOutput();
	ss >> ch;
	if (ch == '?'){
		int src;
		ss >> src;
		int x = get_Register(src);
		ss.clear();
		ss << x;
		D -> write(ss.toUtf8());
	}
	else if (ch == '*'){
		
	}
	else
		qDebug << "D Error :" << ch;
}

void CPU :: Forward_Deal(){
	e_set_cc = (E_icode == IOPL)
		&& !(W_stat == SADR || W_stat == SINS || W_stat == SHLT)
		&& !(M_stat == SADR || M_stat == SINS || M_stat == SHLT)
		&& !(m_stat == SADR || m_stat == SINS || m_stat == SHLT);
	
	if (e_set_cc)
		
	
	if (D_marked_A_e && d_srcA == e_dstE)
		d_valA = e_valE;
	else if (D_marked_A_m && d_srcA == M_dstM)
		d_valA = m_valM;
	if (D_marked_B_e && d_srcB == e_dstE)
		d_valB = e_valE;
	else if (D_marked_B_m && d_srcA == M_dstM)
		d_valA = m_valM;
}

void CPU :: FFF(){
	Fetch();
	Decode();
	Execute();
	Memory();
	Write();
	Forward_Deal();
}
