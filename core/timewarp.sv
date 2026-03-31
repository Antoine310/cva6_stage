// Module protection hit/read
/*
Utiliser dans le cadre : 
    chaine : riscv-none-elf-gcc
    GCC Version : 13.1.0
    Verilator Version : Verilator 5.008
    Config : cv64a6_imafdc_sv39
    ISA : rv64gc_zba_zbb_zbs_zbc

Commande test : 
    python3 cva6.py   --target cv64a6_imafdc_sv39   --iss "$DV_SIMULATORS"   --iss_yaml cva6.yaml   --c_tests ../tests/custom/hello_world/instr.c   
    --linker ../../config/gen_from_riscv_config/linker/link.ld   --gcc_opts='-static -mcmodel=medany -fvisibility=hidden -nostdlib -nostartfiles -g 
    ../tests/custom/common/syscalls.c ../tests/custom/common/crt.S -lgcc -I../tests/custom/env -I../tests/custom/common'   --issrun_opts='+echo_uart'

*/
module timewarp
  import ariane_pkg::*;
#(
    parameter config_pkg::cva6_cfg_t CVA6Cfg = config_pkg::cva6_cfg_empty,
    parameter type dcache_req_o_t = logic,
    parameter int HIT_TIME = 10,    // Delais Hit présent 
    parameter int STALL_COMMIT= 10, // temps Stall + 1 
    parameter int PREDIC= 30 // Temps entre deux lecture de cycle pour augmenter la predic
) (
    // Subsystem Clock - SUBSYSTEM
    input logic clk_i,
    // Asynchronous reset active low - SUBSYSTEM
    input logic rst_ni,
    // Lecture du cycle en cours dans csr_regfile 
    input logic csr_lecture_cycle,
    // HIT du Dcache avant commit 
    input logic dcache_hit_i,
    // Load commit 
    input logic load_commit_i,
    // Load invalid du commit  
    input logic load_invalid_i,
    // Lock le commit 
    output logic protect_en_o
);
    logic [$clog2(HIT_TIME+1)-1:0] compteur_hit;
    logic [$clog2(STALL_COMMIT+1)-1:0] compteur_stall;
    logic [$clog2(PREDIC+1)-1:0] compteur_predicte;

    logic hit_en ; // Signal d'un Load Hit en cours, Attente si une lecture va s'effectuer dessus 
    logic [2:0] dcache_hit_q; // Compteur pour prendre en compte les load Hit pas encore arriver au commit
    logic [2:0] predicteur;
    logic csr_lecture_en;
    logic predic_positive;

    assign predic_positive = (csr_lecture_en && predicteur > 0) || (predicteur > 1);

    always_comb begin : activation_stall

        protect_en_o = 1'b0; 
        // Si on eu un load hit et une lecture, on commence un stall du pipeline.
        if (hit_en && csr_lecture_cycle && predic_positive ) begin 
            protect_en_o = 1'b1; 
            $display("[cycle %0d] Time Protection \n", nb_cycle);

        // On continue le temps du compteur.
        end else if (compteur_stall !=  0) begin
            protect_en_o = 1'b1;
        end 
    end
    
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (~rst_ni) begin
            hit_en <= 1'b0;
            compteur_hit <= '0;
            compteur_stall <= '0;
            dcache_hit_q <= '0;
            predicteur <= 2'b00;
            csr_lecture_en <= 1'b0;
        end else begin 
            // Compteur des load hit en vol 
            dcache_hit_q <= dcache_hit_q + dcache_hit_i - ((dcache_hit_q>0) && load_commit_i) - ((dcache_hit_q>0) && load_invalid_i);
            
            // Si on a un load commit et que c'était un hit, on commence le compteur et on active le hit_en.
            if ((dcache_hit_q>0) && load_commit_i) begin
                hit_en <= 1'b1; 
                compteur_hit <= HIT_TIME[$bits(compteur_hit)-1:0];
            end else if (compteur_hit != 0) begin
                compteur_hit <= compteur_hit - 1 ; 
                if (compteur_hit == 1) begin
                    hit_en <= 1'b0; 
                end
            end 
            // Si on a une Csr lecture commit et un hit_en, on consomme le Hit et on demarre le stall du pipeline pendant x cycle.
            if (hit_en && csr_lectrue_cycle  && predic_positive ) begin 
                hit_en <= 1'b0; 
                compteur_stall <= STALL_COMMIT[$bits(compteur_stall)-1:0];
            end else if (compteur_stall !=  0) begin
                compteur_stall <= compteur_stall - 1 ; 
            end 

            if (csr_lecture_cycle && !csr_lecture_en) begin 
                csr_lecture_en  <= 1'b1;
                compteur_predicte <= PREDIC[$bits(compteur_predicte)-1:0];
            end else if (csr_lecture_cycle && csr_lecture_en && hit_en) begin 
                if (predicteur < 3)
                    predicteur <= predicteur + 1; 
                csr_lecture_en <= 1'b0; 
                compteur_predicte <= '0;
            end else if (compteur_predicte != 0) begin
                compteur_predicte <= compteur_predicte - 1 ; 
                if (compteur_predicte == 1) begin
                    csr_lecture_en <= 1'b0; 
                    if (predicteur > 0)
                        predicteur <= predicteur - 1; 
                end
            end 
        end 
    end

    logic protect_en_q;
    logic hit_en_q;
    logic csr_cycle_q;
    logic [2:0] dcache_hit_c;    
    int nb_cycle;
    logic load_commit_q;
    logic csr_lecture_en_q;
    logic [2:0] predicteur_q;
    logic predic_positive_q;
    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            protect_en_q  <= 1'b0;
            hit_en_q      <= 1'b0;
            dcache_hit_c  <= '0;
            csr_cycle_q   <= 1'b0;
            nb_cycle      <= 0;
            csr_lecture_en_q <= 1'b0;
            predicteur_q <= '0;
            predic_positive_q <= '0;
        end else begin

            if (predicteur_q != predicteur)
                $display("[cycle %0d] predicteur -> %0d", nb_cycle, predicteur);
            if (predic_positive_q != predic_positive)
                $display("[cycle %0d] predic_positive -> %0d", nb_cycle, predic_positive);

            if (csr_lecture_en_q != csr_lecture_en)
                $display("[cycle %0d] csr_lecture_en -> %0d", nb_cycle, csr_lecture_en);

       
            if (dcache_hit_c != dcache_hit_q)
                $display("[cycle %0d] dcache_hit_cnt -> %0d", nb_cycle, dcache_hit_q);
        
            if (protect_en_o != protect_en_q)
                $display("[cycle %0d] protect_en_o -> %0b \n", nb_cycle, protect_en_o);

            if (hit_en != hit_en_q)
                $display("[cycle %0d] hit_en -> %0b \n ", nb_cycle, hit_en);

            if (csr_lecture_cycle != csr_cycle_q)
                $display("[cycle %0d] csr_lecture_cycle -> %0b \n" , nb_cycle, csr_lecture_cycle ,  );

            if (load_commit_i != load_commit_q)
                $display("[cycle %0d] load_commit_i -> %0b \n", nb_cycle, load_commit_i);

            predicteur_q <= predicteur;
            csr_lecture_en_q <= csr_lecture_en; 
            protect_en_q <= protect_en_o;
            hit_en_q <= hit_en;
            csr_cycle_q <= csr_lecture_cycle;
            dcache_hit_c <= dcache_hit_q;
            load_commit_q <= load_commit_i;
            nb_cycle <= nb_cycle + 1;

        end
    end

endmodule 
