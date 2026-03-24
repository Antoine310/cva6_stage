// Module protection miss/hit

module timewarp
  import ariane_pkg::*;
#(
    parameter config_pkg::cva6_cfg_t CVA6Cfg = config_pkg::cva6_cfg_empty,
    parameter type dcache_req_o_t = logic,
    parameter int HIT_TIME = 10,    // Delais Hit présent 
    parameter int STALL_COMMIT= 10 // temps Stall + 1 
) (
    // Subsystem Clock - SUBSYSTEM
    input logic clk_i,
    // Asynchronous reset active low - SUBSYSTEM
    input logic rst_ni,
    // Lecture du cycle en cours dans csr_regfile 
    input logic csr_lecture_cycle,
    // HIT du Dcache 
    input logic dcache_hit_i,
    // Load commit 
    input logic load_commit_i,
    // Load invalid du commit  
    input logic load_invalid_i,
    // Ex_stage lecture csr 
    input logic lecture_csr_i,
    // Charge cycle csr_regfile
    output logic [6:0] charge_o
);
    logic [$clog2(HIT_TIME+1)-1:0] compteur_hit;
    logic [8:0] compteur_stall;

    logic hit_en ;
    logic [2:0] dcache_hit_q;

    logic [6:0] charge_q, charge_d;    
    logic reset_charge;
    logic csr_lecture;

    always_comb begin : charge

        charge_d = charge_q;
        
        if (csr_lecture && (hit_en)) begin 
            charge_d = charge_q + 7'd10; 
            $display("[cycle %0d] Augmentation de la charge +10\n", nb_cycle);
        end else if (reset_charge) begin
            charge_d = '0;
            $display("[cycle %0d] Reset de la charge\n", nb_cycle);
        end

    end 

    assign charge_o = charge_d ; 

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (~rst_ni) begin
            hit_en <= 1'b0;
            compteur_hit <= '0;
            compteur_stall <= '0;
            dcache_hit_q <= '0;
            charge_q  <= '0;
            reset_charge <= 1'b0;
            csr_lecture <= 1'b0;
        end else begin 

            dcache_hit_q <= dcache_hit_q + dcache_hit_i - ((dcache_hit_q>0) && load_commit_i) - ((dcache_hit_q>0) && load_invalid_i);
            charge_q  <= charge_d;

            if (lecture_csr_i)
                csr_lecture <= 1'b1;
            else if ((csr_lecture && (hit_en)) || csr_lecture_cycle)
                csr_lecture <= 1'b0;
                
            if ((dcache_hit_q > 0) && load_commit_i) begin
                hit_en <= 1'b1; 
                compteur_hit <= HIT_TIME[$bits(compteur_hit)-1:0];
                $display("[cycle %0d] Lancer du Timer HIT  \n", nb_cycle);
            end else if (compteur_hit != 0) begin
                compteur_hit <= compteur_hit - 1 ; 
                if (compteur_hit == 1) begin
                    hit_en <= 1'b0; 
                end
            end 

            reset_charge <= 1'b0;

            if (hit_en && csr_lecture_cycle) begin 
                hit_en <= 1'b0; 
                compteur_stall <= STALL_COMMIT[$bits(compteur_stall)-1:0] + charge_d;
                $display("[cycle %0d] Lancer du Timer 1 \n", nb_cycle);
            end else if (csr_lecture_cycle && compteur_stall>0) begin
                compteur_stall <= STALL_COMMIT[$bits(compteur_stall)-1:0] + charge_d ; // + charge ? ou pas 
                $display("[cycle %0d] Relance du Timer reset juste lecture \n", nb_cycle);
            end else if (compteur_stall !=  0) begin
                compteur_stall <= compteur_stall - 1 ; 
                if (compteur_stall == 1) begin
                    reset_charge <= 1'b1; 
                end
            end 
        end 
    end

    logic hit_en_q;
    logic csr_cycle_q;
    logic [2:0] dcache_hit_c;    
    int nb_cycle;
    logic load_commit_q;
    logic lecture_csr_i_q;
    logic csr_lecture_q;

    always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
            hit_en_q      <= 1'b0;
            dcache_hit_c  <= '0;
            csr_cycle_q   <= 1'b0;
            nb_cycle      <= 0;
            csr_lecture_q <= 1'b0;
            lecture_csr_i_q <= 1'b0;
        end else begin
            if (csr_lecture_q != csr_lecture)
                $display("[cycle %0d] csr_lecture -> %0d", nb_cycle, csr_lecture);
            if (lecture_csr_i_q != lecture_csr_i)
                $display("[cycle %0d] lecture_csr_i_q -> %0d", nb_cycle, lecture_csr_i);

            if (dcache_hit_c != dcache_hit_q)
                $display("[cycle %0d] dcache_hit_cnt -> %0d", nb_cycle, dcache_hit_q);

            if (hit_en != hit_en_q)
                $display("[cycle %0d] hit_en -> %0b \n ", nb_cycle, hit_en);

            if (csr_lecture_cycle != csr_cycle_q)
                $display("[cycle %0d] csr_lecture_cycle -> %0b \n" , nb_cycle, csr_lecture_cycle);

            if (load_commit_i != load_commit_q)
                $display("[cycle %0d] load_commit_i -> %0b \n", nb_cycle, load_commit_i);

            if (load_commit_i && load_invalid_i )
                $display("[cycle %0d] erreur load valid et invalid !\n", nb_cycle);

            if (charge_o != charge_q)
                $display("[cycle %0d] charge_o -> %0d", nb_cycle, charge_o);

            if (charge_d != charge_q)
                $display("[cycle %0d] charge_q=%0d charge_d=%0d charge_o=%0d",
                        nb_cycle, charge_q, charge_d, charge_o);

            hit_en_q <= hit_en;
            csr_cycle_q <= csr_lecture_cycle;
            dcache_hit_c <= dcache_hit_q;
            load_commit_q <= load_commit_i;
            csr_lecture_q <= csr_lecture;
            lecture_csr_i_q <= lecture_csr_i;
            
            nb_cycle <= nb_cycle + 1;

        end
    end

endmodule 
