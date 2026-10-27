-- Start each chapter and appendix on a fresh page in the reading edition.
-- This is presentation-only: the authoritative Markdown remains unchanged.
function Header(el)
  if el.level == 1 then
    return {
      pandoc.RawBlock("latex", "\\clearpage"),
      el
    }
  end
end
